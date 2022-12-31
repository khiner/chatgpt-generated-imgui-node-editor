#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include <imgui_internal.h>

#include <unordered_map>
#include <vector>

namespace ImGui {

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

template<typename T>
struct Node {
    ImVec2 Pos;
    T Data;
    std::vector<int> Inputs;
    std::vector<int> Outputs;

    // Draw the node
    void Draw(ImDrawList *draw_list) {
        const ImVec2 node_rect_min = Pos;
        const ImVec2 node_rect_max = Pos + NodeSize;

        // Draw node box
        draw_list->AddRectFilled(node_rect_min, node_rect_max, NodeColor, NodeRounding);
        draw_list->AddRect(node_rect_min, node_rect_max, NodeOutlineColor, NodeRounding);

        // Draw input sockets
        for (int socket_index = 0; socket_index < Inputs.size(); socket_index++) {
            ImVec2 socket_pos = node_rect_min + ImVec2(0, SocketYSpacing * socket_index);
            draw_list->AddCircleFilled(socket_pos, SocketRadius, SocketColor);
            draw_list->AddCircle(socket_pos, SocketRadius, SocketOutlineColor);
        }

        // Draw output sockets
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

    // Draw the node
    void Draw(ImDrawList *draw_list) {
        if (LayoutType == TwoChildNodeLayoutType::Parallel) {
            // Draw child nodes vertically
            Child1.Pos = ImVec2(Child1.Pos.x, Child1.Pos.y);
            Child2.Pos = ImVec2(Child2.Pos.x, Child1.Pos.y + NodeSize.y);
            Child1.Draw(draw_list);
            Child2.Draw(draw_list);
        } else if (LayoutType == TwoChildNodeLayoutType::Sequential) {
            // Draw child nodes horizontally
            Child1.Pos = ImVec2(Child1.Pos.x, Child1.Pos.y);
            Child2.Pos = ImVec2(Child1.Pos.x + NodeSize.x, Child1.Pos.y);
            Child1.Draw(draw_list);
            Child2.Draw(draw_list);
        }
    }
};

template<typename T>
struct Connections {
    std::unordered_map<int, ImVec2> connections;

    // Draw connections
    void Draw(ImDrawList *draw_list, const std::vector<Node<T>> &nodes) {
        for (auto &connection : connections) {
            int output_index = connection.first;
            int input_index = connection.second.x;
            ImVec2 output_pos = nodes[output_index].Pos;
            ImVec2 input_pos = connection.second;
            output_pos.x += CurveControlPointXOffset;
            output_pos.y += SocketYSpacing * (output_index % NodeIdMod);
            input_pos.x += CurveControlPointXOffset;
            input_pos.y += SocketYSpacing * (input_index % NodeIdMod);
            draw_list->AddBezierCurve(
                output_pos,
                output_pos + ImVec2(CurveControlPointXOffset, 0),
                input_pos + ImVec2(-CurveControlPointXOffset, 0),
                input_pos,
                CurveColor,
                BezierCurveWidth
            );
        }
    }
};

template<typename T>
struct Graph {
    std::vector<Node<T>> nodes;
    Connections<T> connections;

    // Find the nearest socket to a given position
    bool FindSocket(ImVec2 pos, int &node_id, int &socket_id) {
        for (int node_index = 0; node_index < nodes.size(); node_index++) {
            auto &node = nodes[node_index];
            ImVec2 node_rect_min = node.Pos;
            ImVec2 node_rect_max = node.Pos + NodeSize;
            if (pos.x >= node_rect_min.x && pos.y >= node_rect_min.y && pos.x <= node_rect_max.x && pos.y <= node_rect_max.y) {
                if (pos.x - node_rect_min.x < SocketRadius) {
                    socket_id = (pos.y - node_rect_min.y) / SocketYSpacing;
                    if (socket_id >= 0 && socket_id < node.Inputs.size()) {
                        node_id = node_index;
                        return true;
                    }
                } else if (node_rect_max.x - pos.x < SocketRadius) {
                    socket_id = (pos.y - node_rect_min.y) / SocketYSpacing;
                    if (socket_id >= 0 && socket_id < node.Outputs.size()) {
                        node_id = node_index;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // Draw the graph
    void Draw(ImDrawList *draw_list) {
        for (auto &node : nodes) {
            node.Draw(draw_list);
        }
        connections.Draw(draw_list, nodes);
    }
};

} // namespace ImGui

int main(int argc, char **argv) {
    // Initialize SDL and ImGui
    SDL_Init(SDL_INIT_EVERYTHING);
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplSDL2_InitForOpenGL(SDL_GL_GetCurrentWindow(), SDL_GL_GetCurrentContext());
    ImGui_ImplOpenGL3_Init("#version 330");

    // Create a node graph
    ImGui::Graph<int> graph;
    graph.nodes.emplace_back(ImGui::Node<int>{
        ImVec2(100.0f, 100.0f),
        0,
        {0},
        {1}});
    graph.nodes.emplace_back(ImGui::Node<int>{
        ImVec2(300.0f, 100.0f),
        1,
        {1},
        {0}});
    graph.connections.connections = {
        {0, ImVec2(1, 0)}};

    // Create a two-child node
    ImGui::TwoChildNode<int> two_child_node{
        ImGui::TwoChildNodeLayoutType::Sequential,
        ImGui::Node<int>{
            ImVec2(100.0f, 300.0f),
            2,
            {0},
            {0, 1}},
        ImGui::Node<int>{
            ImVec2(250.0f, 300.0f),
            3,
            {1},
            {0}}};

    // Main loop
    bool done = false;
    while (!done) {
        // Poll and handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                done = true;
            }
        }

        // Start a new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(SDL_GL_GetCurrentWindow());
        ImGui::NewFrame();

        // Draw the node graph
        graph.Draw(ImGui::GetBackgroundDrawList());

        // Draw the two-child node
        two_child_node.Draw(ImGui::GetBackgroundDrawList());

        // Render the GUI
        ImGui::Render();
        SDL_GL_MakeCurrent(SDL_GL_GetCurrentWindow(), SDL_GL_GetCurrentContext());
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(SDL_GL_GetCurrentWindow());
    }

    // Clean up
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_Quit();

    return 0;
}
