#include <pscript/context.hpp>

#include <imgui/imgui.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "external/imgui_impl_glfw.h"
#include "external/imgui_impl_opengl3.h"
#include "implot.h"

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

bool imgui_button(ps::string_type const& str) {
    return ImGui::Button(str.representation().c_str());
}

ImPlotPoint list_data_getter(void* data, int idx) {
    ps::list_type const* list = (ps::list_type const*)data;
    auto const& vec = list->representation();
    ps::value const& val = vec[idx];

    ps::struct_type const& struc = static_cast<ps::struct_type const&>(val);
    ps::value const& x = struc.access("x");
    ps::value const& y = struc.access("y");

    ImPlotPoint point {};
    point.x = x.real_value().value();
    point.y = y.real_value().value();
    return point;
}

bool imgui_begin_plot(ps::string_type const& str) {
    return ImPlot::BeginPlot(str.representation().c_str());
}

int imgui_plot_scatter(ps::string_type const& str, ps::list_type const& data) {
    ImPlot::PlotScatterG(str.representation().c_str(), list_data_getter, (void*) &data, data.size());
    return 0;
}

int imgui_plot_line(ps::string_type const& str, ps::list_type const& data) {
    ImPlot::PlotLineG(str.representation().c_str(), list_data_getter, (void*) &data, data.size());
    return 0;
}

int plot_next_style(int r, int g, int b) {
    ImPlot::SetNextFillStyle(ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f));
    return 0;
}

int plot_next_marker_style(float size, int r, int g, int b) {
    auto color = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    ImPlot::SetNextMarkerStyle(IMPLOT_AUTO, size, color, IMPLOT_AUTO, color);
    return 0;
}

int imgui_same_line() {
    ImGui::SameLine();
    return 0;
}

// data_ref is of type imgui.Reference
bool imgui_input_float(ps::string_type const& id, float& data_ref) {
    return ImGui::InputFloat(id.representation().c_str(), &data_ref);
}

int imgui_end_plot() {
    ImPlot::EndPlot();
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
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    constexpr size_t memsize = 1024 * 1024;
    ps::context ctx(memsize);

    ps::script build_ui = ps::script(read_file("ps/ui_frame.ps"), ctx);
    ps::script perceptron = ps::script(read_file("ps/perceptron.ps"), ctx);

    extern_library lib {};
    lib.add_function(ctx, "imgui.begin", &ps_bindings::imgui_begin);
    lib.add_function(ctx, "imgui.end", &ps_bindings::imgui_end);
    lib.add_function(ctx, "imgui.button", &ps_bindings::imgui_button);
    lib.add_function(ctx, "imgui.begin_plot", &ps_bindings::imgui_begin_plot);
    lib.add_function(ctx, "imgui.plot_scatter", &ps_bindings::imgui_plot_scatter);
    lib.add_function(ctx, "imgui.plot_line", &ps_bindings::imgui_plot_line);
    lib.add_function(ctx, "imgui.end_plot", &ps_bindings::imgui_end_plot);
    lib.add_function(ctx, "imgui.next_plot_style", &ps_bindings::plot_next_style);
    lib.add_function(ctx, "imgui.next_plot_marker_style", &ps_bindings::plot_next_marker_style);
    lib.add_function(ctx, "imgui.same_line", &ps_bindings::imgui_same_line);
    lib.add_function(ctx, "imgui.input_float", &ps_bindings::imgui_input_float);

    ps::execution_context exec {};
    exec.module_paths.emplace_back("ps/");
    exec.externs = &lib;

    ctx.execute(perceptron, exec);

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ctx.execute(build_ui, exec);

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}