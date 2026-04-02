# Project: 
LoreForge

## Description: 
Library that can parse multiple language code bases (C++, python, typescript, javascript, etc) to understand their intricate structure, extract rich code intelligence (e.g., function signatures, call graphs), generate embeddings for semantic search, and utilize an LLM to answer complex questions about functionality and call graphs as well as overall codebase structure. The repo also includes a standalone MCP server that can be used to host the library and provide a REST API for clients to interact with the library.

## Primary Goals: 
- Create a c/c++ library that can parse multiple coding languages.
- Provide enrichment to the code base by
  - Determining structure generate call graphs and map function and data and make it easly searchable
  - Read source and use LLMs to translate code into concise and descriptive summaries.

## Secondary Goal: 
- Create an MCP server

## Target Audience: 
C++ developers, data scientists, and engineers building applications that require code base understanding.

##  Core Features:
- Multi language parsers and AST generator, Traverses source code files, identifies key structures (functions, classes), and extracts their content and metadata.
- Chunking & AST Analysis, reaks down code into meaningful semantic chunks (e.g., functions, methods) for embedding. Analyzes the Abstract Syntax Tree (AST) to build call graphs.
- Embedding & Vector search, Converts code chunks into numerical vector representations which can be searched to provide similarity and proximatty results.

## Additional Documentation:
- docs/developer.md - Contains more detailed information on the project including structure, interface, configuration, etc...