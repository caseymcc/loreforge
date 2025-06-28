#include "crow.h"
#include <tree_sitter/api.h>
#include <faiss/Index.h>
#include <faiss/IndexFlat.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>

// Forward declaration of the C++ and Python grammars
extern "C" TSLanguage *tree_sitter_cpp();
extern "C" TSLanguage *tree_sitter_python();

void parse_and_print_definitions(const std::string& code, TSLanguage* language) {
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, language);

    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        code.c_str(),
        code.length()
    );

    TSNode root_node = ts_tree_root_node(tree);
    TSQuery *query;
    uint32_t error_offset;
    TSQueryError error_type;

    if (language == tree_sitter_cpp()) {
        const char* cpp_query_string = "[(function_definition) @function (class_specifier name: (type_identifier) @class)]";
        query = ts_query_new(
            language,
            cpp_query_string,
            strlen(cpp_query_string),
            &error_offset,
            &error_type
        );
    } else {
        const char* python_query_string = "[(function_definition name: (identifier) @function) (class_definition name: (identifier) @class)]";
        query = ts_query_new(
            language,
            python_query_string,
            strlen(python_query_string),
            &error_offset,
            &error_type
        );
    }

    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, query, root_node);

    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match)) {
        for (uint32_t i = 0; i < match.capture_count; ++i) {
            TSNode captured_node = match.captures[i].node;
            uint32_t length = ts_node_end_byte(captured_node) - ts_node_start_byte(captured_node);
            std::string name(code.substr(ts_node_start_byte(captured_node), length));
            std::cout << name << std::endl;
        }
    }

    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

int main(int argc, char* argv[])
{
    if (argc > 1) {
        std::string filepath = argv[1];
        std::ifstream file(filepath);
        if (!file) {
            std::cerr << "Error opening file: " << filepath << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string code = buffer.str();

        if (filepath.rfind(".cpp") != std::string::npos) {
            parse_and_print_definitions(code, tree_sitter_cpp());
        } else if (filepath.rfind(".py") != std::string::npos) {
            parse_and_print_definitions(code, tree_sitter_python());
        } else {
            std::cerr << "Unsupported file type: " << filepath << std::endl;
            return 1;
        }
        return 0;
    }

    crow::SimpleApp app;

    // --- Faiss Index Setup ---
    int dimension = 128; // Example dimension for embeddings
    faiss::IndexFlatL2 index(dimension);
    std::cout << "Faiss index initialized with dimension " << dimension << std::endl;
    std::cout << "Index is_trained: " << (index.is_trained ? "true" : "false") << std::endl;
    std::cout << "Index ntotal: " << index.ntotal << std::endl;
    // -------------------------

    CROW_ROUTE(app, "/health")([]()
    {
        return "Server is running!";
    });

    CROW_ROUTE(app, "/parse")
        .methods("POST"_method)
        ([&](const crow::request& req){ // Capture index by reference
            std::string code = req.body;
            if (code.empty()) {
                return crow::response(400, "No code provided to parse.");
            }

            // Set up the tree-sitter parser
            TSParser *parser = ts_parser_new();
            ts_parser_set_language(parser, tree_sitter_cpp());

            // Parse the code
            TSTree *tree = ts_parser_parse_string(
                parser,
                NULL,
                code.c_str(),
                code.length()
            );

            // Get the root node of the syntax tree
            TSNode root_node = ts_tree_root_node(tree);

            // --- Placeholder for chunking and embedding ---
            // In a real implementation, you would traverse the tree,
            // extract meaningful chunks (functions, classes),
            // generate embeddings for them, and add them to the index.
            // For now, let's just add a dummy vector.
            if (index.ntotal < 100) { // Add up to 100 dummy vectors
                std::vector<float> dummy_embedding(dimension);
                for(int i = 0; i < dimension; ++i) {
                    dummy_embedding[i] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                }
                index.add(1, dummy_embedding.data());
                std::cout << "Added a dummy vector to the index. Total vectors: " << index.ntotal << std::endl;
            }
            // --------------------------------------------

            char *string = ts_node_string(root_node);
            std::string result(string);
            free(string);

            // Clean up
            ts_tree_delete(tree);
            ts_parser_delete(parser);

            return crow::response(200, result);
        });

    CROW_ROUTE(app, "/search")
        .methods("POST"_method)
        ([&](const crow::request& req){ // Capture index by reference
            if (index.ntotal == 0) {
                return crow::response(503, "Index is not ready or is empty.");
            }

            // --- Placeholder for search ---
            // In a real implementation, you would generate an embedding
            // for the query text from req.body.
            // For now, let's just search with a dummy vector.
            std::vector<float> query_embedding(dimension);
            for(int i = 0; i < dimension; ++i) {
                query_embedding[i] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            }

            int k = 5; // Number of nearest neighbors to search for
            std::vector<faiss::Index::idx_t> labels(k);
            std::vector<float> distances(k);

            index.search(1, query_embedding.data(), k, distances.data(), labels.data());

            // Format the results
            crow::json::wvalue result;
            for(int i = 0; i < k; ++i) {
                result["results"][i]["id"] = labels[i];
                result["results"][i]["distance"] = distances[i];
            }
            // ----------------------------

            return crow::response(200, result);
        });


    app.port(18080).multithreaded().run();
}