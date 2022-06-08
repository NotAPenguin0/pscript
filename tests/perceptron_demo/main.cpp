#include <pscript/context.hpp>

#include <imgui/imgui.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "external/imgui_impl_glfw.h"
#include "external/imgui_impl_opengl3.h"

#include <imgui/imgui.h>

#include <string>
#include <fstream>
#include <unordered_map>

std::string read_file(const char* filename) {
    std::ifstream in { filename };
    return std::string { std::istreambuf_iterator<char>{in}, {} };
}


class extern_library : public ps::extern_function_library {
public:
    template<typename C>
    void add_function(ps::context& ctx, std::string const& name, C&& callable) {
        functions.insert({name, plib::make_concrete_function<ps::value>(callable, [&ctx](auto x){
            return ps::value::from(ctx.memory(), x);
        })});
    }

    plib::erased_function<ps::value>* get_function(std::string const& name) override {
        return functions.at(name);
    }

    ~extern_library() {
        for (auto& [k, v] : functions) {
            delete v;
        }
        functions.clear();
    }

private:
    std::unordered_map<std::string, plib::erased_function<ps::value>*> functions {};
};

namespace ps_bindings {

bool imgui_begin(ps::string_type const& str) {
    return ImGui::Begin(str.representation().c_str());
}

int imgui_end() {
    ImGui::End();
    return 0;
}

}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* window = glfwCreateWindow(1200, 900, "PScript perceptron demo", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to load gl context.";
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    constexpr size_t memsize = 1024 * 1024;
    ps::context ctx(memsize);

    ps::script main = ps::script(read_file("ps/main.ps"), ctx);

    extern_library lib {};
    lib.add_function(ctx, "imgui.begin", &ps_bindings::imgui_begin);
    lib.add_function(ctx, "imgui.end", &ps_bindings::imgui_end);

    ps::execution_context exec {};
    exec.module_paths.emplace_back("ps/");
    exec.externs = &lib;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ctx.execute(main, exec);

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}