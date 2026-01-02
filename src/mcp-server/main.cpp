#include "loreforge/service.h"
#include "loreforge/globalCodebaseManager.h"

#include <iostream>
#include <string>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class MCPServer {
public:
    MCPServer(const std::string& repositoryPath, const std::string& model)
        : m_service(repositoryPath, model)
    {
    }

    void run() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            
            try {
                json request = json::parse(line);
                json response = handleRequest(request);
                std::cout << response.dump() << std::endl;
            } catch (const std::exception& e) {
                json error = {
                    {"jsonrpc", "2.0"},
                    {"error", {
                        {"code", -32700},
                        {"message", std::string("Parse error: ") + e.what()}
                    }}
                };
                std::cout << error.dump() << std::endl;
            }
        }
    }

private:
    json handleRequest(const json& request) {
        std::string method = request.value("method", "");
        json params = request.value("params", json::object());
        auto id = request.value("id", json());

        json response = {
            {"jsonrpc", "2.0"},
            {"id", id}
        };

        if (method == "initialize") {
            response["result"] = {
                {"protocolVersion", "2024-11-05"},
                {"capabilities", {
                    {"tools", json::object()},
                    {"prompts", json::object()}
                }},
                {"serverInfo", {
                    {"name", "loreforge-mcp"},
                    {"version", "1.0.0"}
                }}
            };
        }
        else if (method == "tools/list") {
            response["result"] = {
                {"tools", {
                    {
                        {"name", "query_codebase"},
                        {"description", "Query the codebase using semantic search"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"query", {
                                    {"type", "string"},
                                    {"description", "The query to search for in the codebase"}
                                }},
                                {"codebase_name", {
                                    {"type", "string"},
                                    {"description", "Name of the codebase to query"}
                                }}
                            }},
                            {"required", {"query"}}
                        }}
                    },
                    {
                        {"name", "index_repository"},
                        {"description", "Index a repository for semantic search"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"repository_path", {
                                    {"type", "string"},
                                    {"description", "Path to the repository to index"}
                                }},
                                {"codebase_name", {
                                    {"type", "string"},
                                    {"description", "Name to assign to this codebase"}
                                }}
                            }},
                            {"required", {"repository_path", "codebase_name"}}
                        }}
                    },
                    {
                        {"name", "parse_file"},
                        {"description", "Parse a file and return its structure"},
                        {"inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"file_path", {
                                    {"type", "string"},
                                    {"description", "Path to the file to parse"}
                                }},
                                {"content", {
                                    {"type", "string"},
                                    {"description", "Optional content to parse instead of reading from file"}
                                }}
                            }},
                            {"required", {"file_path"}}
                        }}
                    }
                    ,
                    {
                        "name", "analyze_function",
                        "description", "Analyze a function to get its signature and details",
                        "inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"file_path", {
                                    {"type", "string"},
                                    {"description", "Path to the file containing the function"}
                                }},
                                {"function_name", {
                                    {"type", "string"},
                                    {"description", "Name of the function to analyze"}
                                }}
                            }},
                            {"required", {"file_path", "function_name"}}
                        }
                    },
                    {
                        "name", "find_similar",
                        "description", "Find code similar to a query",
                        "inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"query", {
                                    {"type", "string"},
                                    {"description", "Query text or code snippet"}
                                }},
                                {"model", {
                                    {"type", "string"},
                                    {"description", "Embedding model to use"}
                                }},
                                {"codebase_name", {
                                    {"type", "string"},
                                    {"description", "Specific codebase to search"}
                                }},
                                {"top_k", {
                                    {"type", "integer"},
                                    {"description", "Number of results to return"}
                                }}
                            }},
                            {"required", {"query"}}
                        }
                    },
                    {
                        "name", "get_call_graph",
                        "description", "Get the call graph for a function",
                        "inputSchema", {
                            {"type", "object"},
                            {"properties", {
                                {"file_path", {
                                    {"type", "string"},
                                    {"description", "Path to the file"}
                                }},
                                {"function_name", {
                                    {"type", "string"},
                                    {"description", "Root function name"}
                                }},
                                {"depth", {
                                    {"type", "integer"},
                                    {"description", "Depth of the call graph"}
                                }}
                            }},
                            {"required", {"file_path", "function_name"}}
                        }
                    }
                }}
            };
        }
        else if (method == "tools/call") {
            std::string toolName = params.value("name", "");
            json arguments = params.value("arguments", json::object());
            
            if (toolName == "query_codebase") {
                response["result"] = queryCodebase(arguments);
            }
            else if (toolName == "index_repository") {
                response["result"] = indexRepository(arguments);
            }
            else if (toolName == "parse_file") {
                response["result"] = parseFile(arguments);
            }
            else if (toolName == "analyze_function") {
                response["result"] = analyzeFunction(arguments);
            }
            else if (toolName == "find_similar") {
                response["result"] = findSimilar(arguments);
            }
            else if (toolName == "get_call_graph") {
                response["result"] = getCallGraph(arguments);
            }
            else {
                response["error"] = {
                    {"code", -32601},
                    {"message", "Tool not found: " + toolName}
                };
            }
        }
        else if (method == "prompts/list") {
            response["result"] = {
                {"prompts", json::array()}
            };
        }
        else {
            response["error"] = {
                {"code", -32601},
                {"message", "Method not found: " + method}
            };
        }

        return response;
    }

    json queryCodebase(const json& arguments) {
        std::string query = arguments.value("query", "");
        std::string codebaseName = arguments.value("codebase_name", "");
        
        // This would need to be implemented using the loreforge service
        // For now, return a placeholder
        return {
            {"content", {
                {
                    {"type", "text"},
                    {"text", "Query functionality would be implemented here using the loreforge service"}
                }
            }}
        };
    }

    json indexRepository(const json& arguments) {
        std::string repositoryPath = arguments.value("repository_path", "");
        std::string codebaseName = arguments.value("codebase_name", "");
        
        // This would need to be implemented using the loreforge service
        return {
            {"content", {
                {
                    {"type", "text"},
                    {"text", "Repository indexing would be implemented here"}
                }
            }}
        };
    }

    json parseFile(const json& arguments) {
        loreforge::ParseFileRequest request;
        request.set_file_path(arguments.value("file_path", ""));
        if (arguments.contains("content")) request.set_content(arguments["content"]);

        loreforge::ParseFileResponse response;
        grpc::Status status = m_service.ParseFile(nullptr, &request, &response);

        if (!status.ok()) {
            return {
                {"isError", true},
                {"content", {{
                    {"type", "text"},
                    {"text", "RPC failed: " + status.error_message()}
                }}}
            };
        }

        return {
            {"content", {{
                {"type", "text"},
                {"text", response.success() ? "File parsed successfully." : "Failed to parse file."}
            }}}
        };
    }

    json analyzeFunction(const json& arguments) {
        loreforge::AnalyzeFunctionRequest request;
        request.set_file_path(arguments.value("file_path", ""));
        request.set_function_name(arguments.value("function_name", ""));

        loreforge::AnalyzeFunctionResponse response;
        grpc::Status status = m_service.AnalyzeFunction(nullptr, &request, &response);

        if (!status.ok()) {
             return {{"isError", true}, {"content", {{{"type", "text"}, {"text", status.error_message()}}}}};
        }

        std::stringstream ss;
        if (response.found()) {
            ss << "Signature: " << response.signature() << "\n";
            ss << "Return Type: " << response.return_type() << "\n";
            ss << "Parameters: ";
            for (const auto& param : response.parameters()) ss << param << ", ";
            ss << "\n";
        } else {
            ss << "Function not found.\n";
        }

        return {{"content", {{{"type", "text"}, {"text", ss.str()}}}}};
    }

    json findSimilar(const json& arguments) {
        loreforge::FindSimilarCodeRequest request;
        request.set_query(arguments.value("query", ""));
        if (arguments.contains("model")) request.set_model(arguments["model"]);
        if (arguments.contains("codebase_name")) request.set_codebase_name(arguments["codebase_name"]);
        if (arguments.contains("top_k")) request.set_top_k(arguments["top_k"]);

        loreforge::FindSimilarCodeResponse response;
        grpc::Status status = m_service.FindSimilarCode(nullptr, &request, &response);

        if (!status.ok()) {
             return {{"isError", true}, {"content", {{{"type", "text"}, {"text", status.error_message()}}}}};
        }

        std::stringstream ss;
        for (const auto& chunk : response.similar_chunks()) {
            ss << "File: " << chunk.file_path() << " (" << chunk.start_line() << "-" << chunk.end_line() << ")\n";
            ss << "Score: " << chunk.score() << "\n";
            ss << "Content:\n" << chunk.content() << "\n\n";
        }

        return {{"content", {{{"type", "text"}, {"text", ss.str()}}}}};
    }

    json getCallGraph(const json& arguments) {
        loreforge::GetCallGraphRequest request;
        request.set_file_path(arguments.value("file_path", ""));
        request.set_function_name(arguments.value("function_name", ""));
        
        loreforge::GetCallGraphResponse response;
        grpc::Status status = m_service.GetCallGraph(nullptr, &request, &response);

        if (!status.ok()) {
             return {{"isError", true}, {"content", {{{"type", "text"}, {"text", status.error_message()}}}}};
        }

        std::stringstream ss;
        if (response.found()) {
             const auto& root = response.root();
             ss << "Root: " << root.function_name() << "\n";
             ss << "Callees:\n";
             for (const auto& callee : root.callees()) {
                 ss << "  - " << callee.function_name() << "\n";
             }
        } else {
            ss << "Graph not found.\n";
        }
         return {{"content", {{{"type", "text"}, {"text", ss.str()}}}}};
    }

    loreforge::Service m_service;
};

int main(int argc, char** argv) {
    std::string repositoryPath = "";
    std::string model = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--repository" && i + 1 < argc) {
            repositoryPath = argv[++i];
        } else if (arg == "--model" && i + 1 < argc) {
            model = argv[++i];
        }
    }

    MCPServer server(repositoryPath, model);
    server.run();

    return 0;
}
