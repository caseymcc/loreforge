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
        std::string filePath = arguments.value("file_path", "");
        std::string content = arguments.value("content", "");
        
        // This would need to be implemented using the loreforge service
        return {
            {"content", {
                {
                    {"type", "text"},
                    {"text", "File parsing would be implemented here"}
                }
            }}
        };
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
