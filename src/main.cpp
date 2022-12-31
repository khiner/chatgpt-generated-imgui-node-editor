#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include <imgui_internal.h>

#include <string>
#include <unordered_map>
#include <vector>

using std::string;
using namespace ImGui;

// Constants for node and connection appearance.
const auto NodeColor = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
const auto NodeOutlineColor = ImColor(0.4f, 0.4f, 0.4f, 1.0f);
const auto SocketColor = ImColor(0.8f, 0.8f, 0.8f, 1.0f);
const auto SocketOutlineColor = ImColor(0.0f, 0.0f, 0.0f, 1.0f);
const auto CurveColor = ImColor(0.8f, 0.8f, 0.8f, 1.0f);
constexpr float NodeRounding = 4.0f;
constexpr float SocketRadius = 4.0f;
constexpr float BezierCurveWidth = 2.0f;
constexpr float CurveControlPointXOffset = 50.0f;
const auto NodeSize = ImVec2(150.0f, 100.0f);
constexpr float SocketYSpacing = 20.0f;
constexpr int NodeIdMod = 5;

// Struct representing a single node in the graph.
template<typename T>
struct Node {
    ImVec2 Pos;
    T Data;
    std::vector<int> Inputs;
    std::vector<int> Outputs;

    // Draw the node.
    void Draw(ImDrawList *draw_list) const {
        const ImVec2 node_rect_min = Pos;
        const ImVec2 node_rect_max = Pos + NodeSize;

        // Draw the box.
        draw_list->AddRectFilled(node_rect_min, node_rect_max, NodeColor, NodeRounding);
        draw_list->AddRect(node_rect_min, node_rect_max, NodeOutlineColor, NodeRounding);

        // Draw the input sockets.
        for (int socket_index = 0; socket_index < Inputs.size(); socket_index++) {
            ImVec2 socket_pos = node_rect_min + ImVec2(0, SocketYSpacing * socket_index);
            draw_list->AddCircleFilled(socket_pos, SocketRadius, SocketColor);
            draw_list->AddCircle(socket_pos, SocketRadius, SocketOutlineColor);
        }

        // Draw the output sockets.
        for (int socket_index = 0; socket_index < Outputs.size(); socket_index++) {
            ImVec2 socket_pos = node_rect_max + ImVec2(0, SocketYSpacing * socket_index);
            draw_list->AddCircleFilled(socket_pos, SocketRadius, SocketColor);
            draw_list->AddCircle(socket_pos, SocketRadius, SocketOutlineColor);
        }
    }
};

enum class TwoChildNodeLayoutType {
    Parallel,
    Sequential,
};

template<typename T>
struct TwoChildNode {
    TwoChildNodeLayoutType LayoutType;
    Node<T> Child1;
    Node<T> Child2;

    // Draw the node.
    void Draw(ImDrawList *draw_list) const {
        if (LayoutType == TwoChildNodeLayoutType::Parallel) {
            // Draw child nodes vertically.
            Child1.Pos = ImVec2(Child1.Pos.x, Child1.Pos.y);
            Child2.Pos = ImVec2(Child2.Pos.x, Child1.Pos.y + NodeSize.y);
            Child1.Draw(draw_list);
            Child2.Draw(draw_list);
        } else if (LayoutType == TwoChildNodeLayoutType::Sequential) {
            // Draw child nodes horizontally.
            Child1.Pos = ImVec2(Child1.Pos.x, Child1.Pos.y);
            Child2.Pos = ImVec2(Child1.Pos.x + NodeSize.x, Child2.Pos.y);
            Child1.Draw(draw_list);
            Child2.Draw(draw_list);
        }
    }
};

template<typename T>
struct Connections {
    std::unordered_map<int, std::vector<int>> Inputs;
    std::unordered_map<int, std::vector<int>> Outputs;

    std::vector<int> &operator[](int node_id) {
        // Check if the node has any inputs or outputs.
        if (Inputs.count(node_id) > 0) {
            return Inputs[node_id];
        } else if (Outputs.count(node_id) > 0) {
            return Outputs[node_id];
        }
        // Otherwise, throw an exception.
        throw std::out_of_range("Node with id '" + std::to_string(node_id) + "' does not exist in Connections struct");
    }

    // Draw connections.
    void Draw(ImDrawList *draw_list, const std::vector<Node<T>> &nodes) const {
        // TODO Implement this function.
    }
};

template<typename T>
struct Graph {
    std::vector<Node<T>> nodes;
    Connections<T> connections;

    void Draw(ImDrawList *draw_list) const {
        for (const auto &node : nodes) {
            node.Draw(draw_list);
        }
        connections.Draw(draw_list, nodes);
    }

    // Find the nearest socket to a given position.
    bool FindSocket(ImVec2 pos, int &node_id, int &socket_id) {
        for (int node_index = 0; node_index < nodes.size(); node_index++) {
            auto &node = nodes[node_index];
            ImVec2 node_rect_min = node.Pos;
            ImVec2 node_rect_max = node.Pos + NodeSize;
            if (pos.x >= node_rect_min.x && pos.y >= node_rect_min.y && pos.x <= node_rect_max.x && pos.y <= node_rect_max.y) {
                if (pos.x - node_rect_min.x < SocketRadius) {
                    socket_id = static_cast<int>((pos.y - node_rect_min.y) / SocketYSpacing);
                    node_id = node_index;
                    return true;
                } else if (node_rect_max.x - pos.x < SocketRadius) {
                    socket_id = static_cast<int>((pos.y - node_rect_min.y) / SocketYSpacing);
                    node_id = node_index;
                    return true;
                }
            }
        }
        return false;
    }

    // Add a connection.
    void AddConnection(int output_index, int input_index) {
        connections.connections[output_index] = ImVec2(input_index, 0);
    }
};

// Populates the provided `window` pointer with an SDL window.
SDL_Window *SetupWindow() {
    // Setup SDL.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return nullptr;
    }

    // Setup window.
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#if __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
    return SDL_CreateWindow("Node Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
}

void Cleanup(SDL_Window *window, SDL_GLContext &gl_context) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int, char **) {
    // Create the graph.
    Graph<int> graph;

    // TODO Construct an example graph.

    SDL_Window *window = SetupWindow();
    if (!window) return 1;

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync.

    // Create the ImGui context.
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsResizeFromEdges = true;

    // Set up ImGui binding.
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 150");

    // Set up style.
    ImGui::StyleColorsDark();

    // Main loop:
    bool done = false;
    while (!done) {
        // Poll and handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }

        // Start the ImGui frame.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        auto *draw_list = GetWindowDrawList();

        // TODO Construct a new window.

        graph.Draw(draw_list); // Draw the node graph.

        // Render the ImGui frame.
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);

        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    Cleanup(window, gl_context);

    return 0;
}
