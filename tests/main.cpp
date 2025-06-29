#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "gtest/gtest.h"
#include "protos/loreforge.grpc.pb.h"

using namespace loreforge;
using namespace grpc;

class LoreForgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        channel = CreateChannel("localhost:50051", InsecureChannelCredentials());
        stub = LoreForge::NewStub(channel);
    }

    std::shared_ptr<Channel> channel;
    std::unique_ptr<LoreForge::Stub> stub;
};

void test_parse_file(
    LoreForge::Stub& stub,
    const std::string& file_path,
    const std::string& content,
    bool expected_success)
{
    ParseFileRequest request;
    request.set_file_path(file_path);
    request.set_content(content);

    ParseFileResponse response;
    ClientContext context;
    Status status = stub.ParseFile(&context, request, &response);

    ASSERT_TRUE(status.ok()) << "RPC failed: " << status.error_code() << ": " << status.error_message();
    ASSERT_EQ(response.success(), expected_success);
}

TEST_F(LoreForgeTest, HealthCheck) {
    HealthCheckRequest request;
    HealthCheckResponse response;
    ClientContext context;
    Status status = stub->HealthCheck(&context, request, &response);

    ASSERT_TRUE(status.ok());
    EXPECT_EQ(response.status(), "SERVING");
}

TEST_F(LoreForgeTest, ParseFile_ValidCpp) {
    test_parse_file(*stub, "sample.cpp", "int main() { return 0; }", true);
}

TEST_F(LoreForgeTest, ParseFile_InvalidCpp) {
    test_parse_file(*stub, "sample.cpp", "int main() { return 0;", false);
}

TEST_F(LoreForgeTest, ParseFile_ValidPython) {
    test_parse_file(*stub, "sample.py", "def main():\n    pass", true);
}

TEST_F(LoreForgeTest, ParseFile_InvalidPython) {
    test_parse_file(*stub, "sample.py", "def main():\nthis is bad\n this is real bad\npass", false);
}

TEST_F(LoreForgeTest, ParseFile_UnsupportedFileType) {
    test_parse_file(*stub, "sample.txt", "this is a plain text file", false);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}