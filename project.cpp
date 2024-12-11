#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <unordered_map>
#include <queue>
// file handling
#include <fstream>
// JSON writing
#include <nlohmann/json.hpp>
// for inf in dijkstras
#include <limits.h>  


using json = nlohmann::json;
using namespace std;

enum class TravelMethod
{
    Foot,
    Cycle,
    Bike,
    Car,
    Bus,
    Train
};

string getTravelMethodName(TravelMethod method)
{
    switch (method)
    {
    case TravelMethod::Foot:
        return "Foot";
    case TravelMethod::Cycle:
        return "Cycle";
    case TravelMethod::Bike:
        return "Bike";
    case TravelMethod::Car:
        return "Car";
    case TravelMethod::Bus:
        return "Bus";
    case TravelMethod::Train:
        return "Train";
    }
    return "Unknown";
}

struct Vertex
{
    string name;
    bool visited;

    Vertex(string name)
    {
        this->name = name;
        this->visited = false;
    }
};

struct TravelMethodStruct
{
    double speed;
    string method_name;
};

struct Edge
{
    int from, to, distance;
    vector<TravelMethodStruct> travel_methods;

    Edge(int from, int to, int distance)
    {
        this->from = from;
        this->to = to;
        this->distance = distance;
    }

    void add_travel_method(TravelMethodStruct travel_method)
    {
        // Check for duplicates
        for (int i = 0; i < travel_methods.size(); ++i)
        {
            if (travel_methods[i].method_name == travel_method.method_name)
            {
                cout << "Travel method " << travel_method.method_name << " already exists" << endl;
                return;
            }
        }

        travel_methods.push_back(travel_method);
    }

    bool contains_travel_method(TravelMethodStruct travel_method)
    {
        for (int i = 0; i < travel_methods.size(); ++i)
        {
            if (travel_methods[i].method_name == travel_method.method_name)
            {
                return true;
            }
        }
        return false;
    }
};


class Graph
{
    vector<Vertex> vertices;
    unordered_map<int, vector<Edge>> adjacent_list;
public:
    void add_vertex(string name) {
        vertices.push_back(Vertex(name));
    }

    vector<Vertex> get_vertices() {
        return vertices;
    }

    unordered_map<int, vector<Edge>> get_adjacent_list() {
        return this->adjacent_list;
    }

    void add_edge(int from, int to, int distance, vector<TravelMethodStruct> travel_methods) {
        _add_edge(from, to, distance, travel_methods);
    }

    void add_edge(int from, int to, int distance, vector<TravelMethodStruct> travel_methods, bool bidirectional) {
        add_edge(from, to, distance, travel_methods);
        if (bidirectional) {
            add_edge(to, from, distance, travel_methods);
        }
    }

    void _add_edge(int from, int to, int distance, vector<TravelMethodStruct> travel_methods) {
        Edge edge(from, to, distance);
        for (TravelMethodStruct travel_method : travel_methods) {
            edge.add_travel_method(travel_method);
        }
        adjacent_list[from].push_back(edge);
    }


    void print_graph() {
        for (int i = 0; i < vertices.size(); i++)
        {
            Vertex vertex = vertices[i];
            cout << "Vertex " << vertex.name << " connects to:" << endl;
            for (Edge edge : adjacent_list.at(i))
            {
                cout << "  Vertex " << vertices[edge.to].name << " via:" << endl;
                for (TravelMethodStruct method : edge.travel_methods)
                {
                    cout << "    Method: " << method.method_name << ", Speed: " << method.speed << endl;
                }
            }
        }
    }

    void to_json(const string &filename) {
        json json_data;
        json_data["nodes"] = json::array();
        json_data["links"] = json::array();

        int v_id = 0;
        for (const Vertex vertex : vertices) {
            json_data["nodes"].push_back({{"id", v_id++}, {"name", vertex.name}});
        }
        
        int e_id = 0;
        for (const auto& pair : adjacent_list) {
            for (const auto& edge : pair.second) {
                json_data["links"].push_back(
                    {
                        {"source", edge.from}, 
                        {"target", edge.to}, 
                        {"weight", edge.distance}
                        }
                        );
            }
        }

        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Failed to open file for writing" << endl;
            return;
        }

        file << json_data.dump(4);
        file.close();
    }

    // helper function to get travel_mode and return its time cost
    double get_travel_time_cost(int distance, TravelMethodStruct travel_mode) {
        double travel_time = distance / travel_mode.speed;
        return travel_time;
    }

    /*
    returns a JSON object with key as dest and value as object with path and cost
    */
    json dijkstra(int start, TravelMethodStruct travel_method) {
        // distances list
        vector<double> distances(vertices.size(), numeric_limits<double>::max());
        // distance to starting node will always be zero
        distances[start] = 0;

        // keep rack of vertex with min travel time. travel_time, vertex
        set<pair<double, int>> vertex_set;
        vertex_set.insert({0, start});
        
        // saves path of each vertex's parent. Will be used to backtrack the path
        vector<int> parent(vertices.size(), -1);

        while (!vertex_set.empty()){
            auto current = vertex_set.begin();

            double current_travel_time = current->first;
            int current_vertex = current->second;

            vertex_set.erase(current);

            /*
            finding paths to connected vertices.If path is less than existing path
            update it.
            */
            for (Edge edge : adjacent_list[current_vertex]) {
                // travel must be possible with given travel method
                if (!edge.contains_travel_method(travel_method)) {
                    continue;
                }

                int connected_vertex = edge.to;
                double connected_vertex_travel_time = get_travel_time_cost(edge.distance, travel_method);

                // calculating new travel_time to the node
                double new_travel_time = current_travel_time + connected_vertex_travel_time;

                // update travel_time if new travel_time is shorter
                if (new_travel_time < distances[connected_vertex]) {
                    // if there are any previous records. Remove them.
                    /*
                    Previousy we might have a record like Till Node A cost is 7 while
                    new calculated cost is 5. So remove all previous records
                    */
                auto temp = vertex_set.find({distances[connected_vertex], connected_vertex});
                if (temp != vertex_set.end()) {
                    vertex_set.erase(temp);
                }
                
                // Update distance and add connected_node to set
                distances[connected_vertex] = new_travel_time;
                vertex_set.insert({new_travel_time, connected_vertex});

                parent[connected_vertex] = current_vertex;
                }
        }
    }

    // Print results
    // cout << "Shortest paths from vertex " << vertices[start].name << ":" << endl;
    // for (int i = 0; i < distances.size(); ++i) {
    //     cout << "To " << vertices[i].name << ": ";
    //     if (distances[i] == numeric_limits<int>::max()) {
    //         cout << "Unreachable" << endl;
    //     } else {
    //         cout << distances[i] << endl;
    //     }
    // }

    json paths_costs = json::object();
    
    for (int i = 0; i < vertices.size(); ++i) {
        // skip current and unaccessible vertex
        if (i == start || distances[i] == numeric_limits<double>::max())
            continue;

        cout << "Path to " << vertices[i].name << ": ";
        vector<int> path;
        for (int v = i; v != -1; v = parent[v]) {
            path.push_back(v);
        }
        // reverse path so we start from source to dest
        reverse(path.begin(), path.end());

        json vertex_paths = json::object();
        vertex_paths["cost"] = distances[i];
        vertex_paths["path"] = json::array();
        
        for (size_t j = 0; j < path.size(); ++j) {
            vertex_paths["path"].push_back(vertices[path[j]].name);
        }
        cout << vertex_paths;
        cout << endl;
        paths_costs[vertices[i].name] = vertex_paths;
    }
    return paths_costs;
}
};

int main()
{
    Graph city_graph;

    TravelMethodStruct foot = {2, "Foot"};
    TravelMethodStruct bike = {30, "Bike"};
    TravelMethodStruct car = {50, "Car"};
    TravelMethodStruct train = {80, "Train"};

    vector<TravelMethodStruct> travel_methods = {foot, bike, car, train};

    city_graph.add_vertex("Kamra");
    city_graph.add_vertex("Attock");
    city_graph.add_vertex("RWP");
    city_graph.add_vertex("ISB");
    city_graph.add_vertex("Peshawar");
    city_graph.add_vertex("Lahore");


    city_graph.add_edge(0, 1, 10, {foot, bike}, true);
    city_graph.add_edge(1, 2, 20, {car}, true);
    city_graph.add_edge(2, 3, 12, {train}, true);
    city_graph.add_edge(0, 3, 11, {foot, car}, true);
    city_graph.add_edge(3, 1, 5, {foot, car}, true);
    city_graph.add_edge(1, 4, 15, {car, train}, true);
    city_graph.add_edge(4, 5, 25, {car}, true);
    city_graph.add_edge(5, 3, 18, {train}, true);
    city_graph.add_edge(1, 5, 18, {train}, true);
    city_graph.add_edge(0, 5, 30, {foot, car}, true);
    city_graph.add_edge(2, 4, 22, {bike, car}, true);

    city_graph.to_json("graph.json");

    ofstream file("paths.json");
    if (!file.is_open()) {
        cerr << "Failed to open file for writing" << endl;
        return 1;
    }
    json json_data = json::object();
    for (int i = 0; i < city_graph.get_vertices().size(); i++) {
        json vertex_data = json::object();
        cout << "Processing " << city_graph.get_vertices()[i].name << endl;
        for (TravelMethodStruct travel_method : travel_methods) {
            json data = json::object();
            data["paths"] = city_graph.dijkstra(i, travel_method);
            if (data["paths"].empty()) {
                continue;
            }
            vertex_data[travel_method.method_name] = data;
        }
        json_data[city_graph.get_vertices()[i].name] = vertex_data;
    }
    file << json_data.dump(4);
    file.close();

    // start a python server
    system("start http://localhost:8000/graph.html && python -m http.server 8000");

    return 0;
}
