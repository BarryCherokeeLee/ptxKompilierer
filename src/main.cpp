#include <iostream>
#include <fstream>
#include <memory>
#include "ast.h"
#include "codegen.h"

extern FILE* yyin;
extern Module* root;
extern int yyparse();

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.ptx> <output.s>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];

    std::cout << "Parsing PTX file: " << inputFile << std::endl;

    FILE* file = fopen(inputFile.c_str(), "r");
    if (!file) {
        std::cerr << "Error: Cannot open file " << inputFile << std::endl;
        return 1;
    }

    yyin = file;
    
    int parseResult = yyparse();
    fclose(file);
    
    if (parseResult != 0) {
        std::cerr << "Parse error occurred" << std::endl;
        return 1;
    }

    if (!root) {
        std::cerr << "Error: No AST generated" << std::endl;
        return 1;
    }

    std::cout << "=== Abstract Syntax Tree ===" << std::endl;
    root->print();

    std::cout << "=== Generating LLVM IR ===" << std::endl;
    
    try {
        std::cout << "Creating PTXCodeGenerator..." << std::endl;
        auto codegen = std::make_unique<PTXCodeGenerator>();
        if (!codegen) {
            std::cerr << "Error: Failed to create code generator" << std::endl;
            return 1;
        }
        std::cout << "PTXCodeGenerator created successfully" << std::endl;
        
        std::cout << "Calling generateCode..." << std::endl;
        codegen->generateCode(root);
        std::cout << "Code generation completed successfully" << std::endl;
        
        std::cout << "Verifying module..." << std::endl;
        if (!codegen->verifyModule()) {
            std::cerr << "Warning: Module verification failed, but continuing..." << std::endl;
        }
        
        std::cout << "Dumping module..." << std::endl;
        codegen->dumpModule();
        
        std::cout << "=== Generating Assembly ===" << std::endl;
        codegen->generateRISCVAssembly(outputFile);
        
        std::cout << "Compilation completed successfully!" << std::endl;
        std::cout << "Output written to: " << outputFile << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in main: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception in main" << std::endl;
        return 1;
    }

    return 0;
}
