#define NOMINMAX
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <stdlib.h>
#include <map>
#define GLFW_INCLUDE_NONE
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <imgui_impl_opengl3_loader.h>
#include "implot.h"
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#error TODO: Emscripten support
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <ranges>
#include <date/date.h>
#include <date/tz.h>

static std::map<std::string, double> combined_data{};


const std::tuple<std::string, std::string> get_dates() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    char buf2[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);
    tstruct.tm_mday = tstruct.tm_mday + 1;
    strftime(buf2, sizeof(buf2), "%Y-%m-%d", &tstruct);
    return std::make_tuple(buf, buf2);
}

std::string date_to_local_hour(std::string time)
{
    std::istringstream in{time.c_str()};
    std::chrono::system_clock::time_point tp;
    in >> date::parse("%FT%T%Ez", tp);

    time_t utctime = (tp.time_since_epoch() / std::chrono::seconds(1));
    struct tm tm = *localtime(&utctime);
    char buf[80];
    strftime(buf, sizeof(buf), "%H:%M", &tm);
    return std::string(buf);
}

uint64_t date_to_timestamp(std::string time)
{
    std::istringstream in{time.c_str()};
    std::chrono::system_clock::time_point tp;
    in >> date::parse("%FT%T%Ez", tp);
    return (tp.time_since_epoch() / std::chrono::seconds(1));
}

bool is_current_hour(uint64_t epoch)
{
    time_t now = time(0);
    time_t time = epoch;
    struct tm tm = *localtime(&time);
    struct tm tm_now = *localtime(&now);

    return tm.tm_mday == tm_now.tm_mday && tm.tm_hour == tm_now.tm_hour;
}

std::string timestamp_to_localtime(uint64_t time_since_epoch)
{
    time_t utctime = time_since_epoch;
    struct tm tm = *localtime(&utctime);
    char buf[80];
    strftime(buf, sizeof(buf), "%H:%M", &tm);
    return std::string(buf);
}

void fetch_data() {
    const auto [now, tomorrow] = get_dates();
    cpr::Response res = cpr::Get(
        cpr::Url{"https://api.mobapp.elektrum.lv/api/v1/market-prices"}, 
        cpr::Parameters{
            /* yyyy-mm-dd */
            {"from_date", now}, 
            {"to_date", tomorrow}
        },
        cpr::Header{{"User-Agent","Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:131.0) Gecko/20100101 Firefox/131.0"}}
    );
    auto json = nlohmann::json::parse(res.text.c_str());
    if(strcmp(json["status"].get<std::string>().c_str(), "OK") == 0)
    {
        combined_data.clear();
        for(const auto& entry : json["data"]["market_prices"])
        {
            combined_data.insert({entry["date"].get<std::string>(), entry["value"].get<double>()});
        }
    }
}


// Main code
int main(int argc, char** argv)
{
    fetch_data();

    // Initialize our UI state
    glfwSetErrorCallback([](int error, const char* description)
    {
#ifdef _WIN32
        MessageBoxA(GetDesktopWindow(), description, "Error", MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);
#else
        fprintf(stderr, "Error %d: %s\n", error, description);
#endif // _WIN32
    });
    if (!glfwInit())
        return EXIT_FAILURE;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(1, 1, "", NULL, NULL);
    if (window == NULL)
        return EXIT_FAILURE;
    glfwMakeContextCurrent(window);

    glfwWaitEventsTimeout(1.0f / 20.0f);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    io.ConfigViewportsNoAutoMerge = true;
    io.MouseDrawCursor = true;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool should_show_demo = true;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = NULL;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //ImGui::ShowDemoWindow();


        ImGui::Begin("Elektrum", &should_show_demo);
        {
            static bool include_vat = true;
            static bool display_as_mwh = false;
            if(ImGui::Button("Atjaunot"))
            {
                fetch_data();
            }
            ImGui::Checkbox("PVN", &include_vat);
            ImGui::Checkbox("EUR/MWh", &display_as_mwh);
            if (ImPlot::BeginPlot("Price history", ImVec2(-1, 0), ImPlotFlags_NoInputs | ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame)) {
                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                ImPlot::SetupAxes("Stundas","centi/kWh", ImPlotAxisFlags_NoSideSwitch);
                auto values = std::views::values(combined_data);
                std::vector<double> prices{ values.begin(), values.end() };
                ImPlot::PlotShaded("Cena", prices.data(), prices.size());
                ImPlot::PlotLine("Cena", prices.data(), prices.size());

                ImPlot::EndPlot();
            }
            if(ImGui::BeginTable("prices", 2, ImGuiTableFlags_Borders))
            {
                ImGui::TableSetupColumn("Stunda");
                ImGui::TableSetupColumn("Cena");
                ImGui::TableHeadersRow();
                for(auto& entry : combined_data)
                {
                    ImGui::TableNextRow();
                    if(is_current_hour(date_to_timestamp(entry.first)))
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 1)));
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", date_to_local_hour(entry.first).c_str());
                    ImGui::TableNextColumn();
                    double val = display_as_mwh ? entry.second : (entry.second / 10);
                    if(include_vat)
                        val *= 1.21;
                    
                    ImGui::Text("%.2f %s", val, display_as_mwh ? "EUR/MWh" : "centi/KWh");
                }
                ImGui::EndTable();
            }

        }
        ImGui::End();
        if(!should_show_demo)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}

#ifdef WIN32
#include <Windows.h>
#include <shellapi.h>

int WINAPI CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    auto argv_utf8 = new char* [argc];
    for (int i = 0; i < argc; i++)
    {
        int requiredSize = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, 0, 0, 0, 0);
        if (requiredSize > 0)
        {
            argv_utf8[i] = new char[requiredSize];
            WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, argv_utf8[i], requiredSize, 0, 0);
        }
        else
        {
            argv_utf8[i] = new char[1];
            argv_utf8[i][0] = '\0';
        }
    }
    LocalFree(argv);
    return main(argc, argv_utf8);
}
#endif // WIN32