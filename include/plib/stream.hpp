#pragma once

#include <plib/types.hpp>
#include <cstring>
#include <cstdlib>

#include <algorithm> // TODO: May want to replace this with a lightweight header for common math functions like min(), with added constexpr support for some

namespace plib {

	namespace detail {

		template<typename T>
		struct stream_chunk {
			T const* pointer = nullptr;
			size_t size = 0;
		};

		// Utility class that abstracts getting the next chunk of data from a stream. Default chunk size is 1024 elements.
		// ChunkSize is in elements, so the size of a chunk in bytes is ChunkSize * sizeof(T)
		template<typename T, size_t ChunkSize>
		class stream_fetcher {
		public:
			using element_type = T;
			static constexpr size_t chunk_size = ChunkSize;

			virtual ~stream_fetcher() = default;
			virtual stream_chunk<T> fetch_chunk() = 0;

			// Returns the size of the read buffer
			virtual size_t buf_size() const = 0;
		};

		template<typename T, size_t ChunkSize>
		class memory_stream_fetcher : public stream_fetcher<T, ChunkSize> {
		public:
			memory_stream_fetcher(T const* pointer, size_t elem_count) {
				begin = cur = pointer;
				size = elem_count;
				end = begin + size;
			}

			stream_chunk<T> fetch_chunk() override {
				// We're at the end of the stream, no more elements can be read
				if (cur == end) { return { .pointer = nullptr, .size = 0 }; }
				// Can't read a full chunk, but there are still some elements
				if (end - cur < ChunkSize) {
					T const* cur_copy = cur;
					cur = end;
					return { .pointer = cur_copy, .size = static_cast<size_t>(cur - cur_copy) };
				}
				// We can read a full chunk
				T const* cur_copy = cur;
				cur += ChunkSize;
				return { .pointer = cur_copy, .size = ChunkSize };
			}
			
			size_t buf_size() const override {
				return size;
			}

		private:
			T const* begin = nullptr;
			T const* end = nullptr;
			T const* cur = nullptr;
			size_t size = 0;
		};

		template<typename T, size_t ChunkSize>
		class file_stream_fetcher : public stream_fetcher<T, ChunkSize> {
		public:
			file_stream_fetcher(char const* path, const char* mode) {
				file = fopen(path, mode);
				// Find file size
				fseek(file, 0, SEEK_END);
				fsize = ftell(file);
				rewind(file);
			}

			~file_stream_fetcher() {
				fclose(file);
			}

			stream_chunk<T> fetch_chunk() override {
				if (feof(file)) { return { .pointer = nullptr, .size = 0 }; }

				auto pos = ftell(file);
				size_t const bytes_to_read = std::min(ChunkSize, fsize - pos);
				fread(&readbuf, sizeof(T), bytes_to_read / sizeof(T), file);

				return { .pointer = readbuf, .size = bytes_to_read / sizeof(T) };
			}

			size_t buf_size() const override {
				return fsize;
			}

		private:
			FILE* file = nullptr;
			size_t fsize = 0;
			T readbuf[ChunkSize];
		};

		template<size_t ChunkSize>
		class binary_input_stream {
		public:
			using element_type = byte;
			static constexpr size_t chunk_size = ChunkSize;

			binary_input_stream() = delete;
			binary_input_stream(binary_input_stream const&) = delete;
			binary_input_stream& operator=(binary_input_stream const&) = delete;

			binary_input_stream(binary_input_stream&& rhs) {
				fetcher = rhs.fetcher;
				current_chunk = rhs.current_chunk;
				offset = rhs.offset;

				rhs.fetcher = nullptr;
				rhs.current_chunk = {};
				rhs.offset = 0;
			}

			binary_input_stream& operator=(binary_input_stream&& rhs) {
				// Checking the fetcher is enough to verify whether this stream is the same one as rhs.
				if (rhs.fetcher != fetcher) {
					fetcher = rhs.fetcher;
					current_chunk = rhs.current_chunk;
					offset = rhs.offset;

					rhs.fetcher = nullptr;
					rhs.current_chunk = {};
					rhs.offset = 0;
				}
				return *this;
			}

			binary_input_stream(owner<detail::stream_fetcher<element_type, ChunkSize>*> fetcher)
				: fetcher(fetcher) {

			}

			static binary_input_stream<ChunkSize> from_memory(byte const* pointer, size_t size) {
				using fetcher_type = detail::memory_stream_fetcher<element_type, ChunkSize>;
				// Create binary input stream
				return binary_input_stream<ChunkSize>(
					// With a memory fetcher. The stream owns this pointer and will delete it on destruction
					new fetcher_type(pointer, size)
				);
			}

			static binary_input_stream<ChunkSize> from_file(const char* path) {
				using fetcher_type = detail::file_stream_fetcher<element_type, ChunkSize>;
				return binary_input_stream<ChunkSize>(
					new fetcher_type(path, "rb") // open file in read-binary mode
				);
			}

			~binary_input_stream() {
				delete fetcher;
			}

			// Reads n bytes into the destination pointer. Returns the amount of bytes copied.
			size_t read_bytes(element_type* dst, size_t n) {
				size_t amount_read = 0;
				while (amount_read != n) {
					size_t max_read_this_chunk = current_chunk.size - offset;
					// If our last chunk was full, or we have never read one, read a new chunk.
					if (current_chunk.pointer == nullptr || max_read_this_chunk == 0) {
						current_chunk = fetcher->fetch_chunk();
						max_read_this_chunk = current_chunk.size;
						offset = 0;
					}
					size_t const to_read_this_chunk = std::min(n - amount_read, max_read_this_chunk);
					std::memcpy(dst + amount_read, current_chunk.pointer + offset, to_read_this_chunk);
					offset += to_read_this_chunk;
					amount_read += to_read_this_chunk;
				}
				return amount_read;
			}

			// Reads n values of type T. Returns the amount of values copied.
			template<typename T>
			size_t read(T* dst, size_t n) {
				return read_bytes(reinterpret_cast<element_type*>(dst), n * sizeof(T)) / sizeof(T);
			}

			// return sthe size of the read buffer
			size_t size() const {
				return fetcher->buf_size();
			}

		private:
			owner<detail::stream_fetcher<byte, ChunkSize>*> fetcher = nullptr;
			// Current chunk
			detail::stream_chunk<element_type> current_chunk{};
			// Current offset in this chunk
			size_t offset = 0;
		};

		template<typename T, size_t ChunkSize>
		class stream_writer {
		public:
			using element_type = T;
			static constexpr size_t chunk_size = ChunkSize;

			virtual ~stream_writer() = default;

			virtual void write_data(T const* pointer, size_t n) = 0;
			virtual void flush() = 0;
		};

		template<typename T, size_t ChunkSize>
		class memory_stream_writer : public stream_writer<T, ChunkSize> {
		public:
			memory_stream_writer(T* pointer, size_t max_size) {
				begin = cur = pointer;
				size = max_size;
				end = begin + size;
			}

			~memory_stream_writer() = default;

			void write_data(T const* pointer, size_t n) override {
				size_t const to_write = std::min(n, end - cur);
				if (to_write == 0) return;
				std::memcpy(cur, pointer, to_write * sizeof(T));
				cur += to_write;
			}

			// No need for flushing in a memory writer
			void flush() override {}

		private:
			T* begin = nullptr;
			T* cur = nullptr;
			T* end = nullptr;
			size_t size = 0;
		};

		template<typename T, size_t ChunkSize>
		class file_stream_writer : public stream_writer<T, ChunkSize> {
		public:
			file_stream_writer(const char* path, const char* mode) {
				file = fopen(path, mode);
			}

			~file_stream_writer() {
				flush();
				fclose(file);
			}

			void write_data(T const* pointer, size_t n) override {
				size_t amount_written = 0;
				while (amount_written != n) {
					// Check if our writebuf is full
					if (offset == ChunkSize) {
						// Write writebuf to file
						write_buf();
						// Reset writebuf. Note that we keep the old contents, we'll simply overwrite these
						offset = 0;
					}

					// Space left in current writebuf
					size_t const space_left = ChunkSize - offset;
					size_t const to_write = std::min(n - amount_written, space_left);
					std::memcpy(writebuf + offset, pointer + amount_written, to_write * sizeof(T));
					offset += to_write;
					amount_written += to_write;
				}
			}

			void flush() override {
				write_buf();
				fflush(file);
			}

		private:
			FILE* file = nullptr;
			T writebuf[ChunkSize];
			size_t offset = 0; // Current offset into the write buffer that is already filled

			void write_buf() {
				// Don't write the full writebuf is it's not entirely filled. To do this we use the offset variable for the elem_count parameter.
				fwrite(writebuf, sizeof(T), offset, file);
			}
		};

		template<size_t ChunkSize>
		class binary_output_stream {
		public:
			using element_type = byte;
			static constexpr size_t chunk_size = ChunkSize;

			binary_output_stream() = delete;
			binary_output_stream(binary_output_stream const&) = delete;
			binary_output_stream& operator=(binary_output_stream const&) = delete;

			binary_output_stream(binary_output_stream&& rhs) {
				writer = rhs.writer;
				rhs.writer = nullptr;
			}

			binary_output_stream& operator=(binary_output_stream&& rhs) {
				// Checking the writer is enough to verify whether this stream is the same one as rhs.
				if (rhs.writer != writer) {
					writer = rhs.writer;
					rhs.writer = nullptr;
				}
				return *this;
			}

			binary_output_stream(owner<detail::stream_writer<element_type, ChunkSize>*> writer)
				: writer(writer) {

			}

			static binary_output_stream<ChunkSize> from_memory(element_type const* pointer, size_t size) {
				using writer_type = detail::memory_stream_writer<element_type, ChunkSize>;
				// Create binary input stream
				return binary_output_stream<ChunkSize>(
					// With a memory writer. The stream owns this pointer and will delete it on destruction
					new writer_type(pointer, size)
				);
			}

			static binary_output_stream<ChunkSize> from_file(const char* path) {
				using writer_type = detail::file_stream_writer<element_type, ChunkSize>;
				return binary_output_stream<ChunkSize>(
					new writer_type(path, "wb") // open file in write-binary mode
				);
			}

			~binary_output_stream() {
				delete writer;
			}

			void flush() {
				writer->flush();
			}

			void write_bytes(element_type const* pointer, size_t n) {
				writer->write_data(pointer, n);
			}

			template<typename T>
			void write(T const* pointer, size_t n) {
				writer->write_data(reinterpret_cast<element_type const*>(pointer), n * sizeof(T));
			}

		private:
			owner<detail::stream_writer<element_type, ChunkSize>*> writer = nullptr;
		};

	} // namespace detail

	using binary_input_stream = detail::binary_input_stream<1024>;
	using binary_output_stream = detail::binary_output_stream<1024>;

} // namespace plib