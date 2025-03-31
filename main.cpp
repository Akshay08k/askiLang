#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>
#include "./generation.hpp"
// Taking Cmd Args Of Custom Lang File
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Incorrect usage. Correct usage is..." << std::endl;
        std::cerr << "a.out <Aski.al>" << std::endl;
        return EXIT_FAILURE;
    }

    // Reading the file and converting it to string
    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1], std::ios::in);
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }

    // Tokenizing using tokenizer and getting back tokens
    Tokenizer tokenizer(std::move(contents));
    std::vector<Token> tokens = tokenizer.tokenize();

    // the tokens which return from tokenizer are passed to parser
    Parser parser(std::move(tokens));
    std::optional<NodeProg> Prog = parser.parseProg();

    // if Program is valid than only go ahead
    // or else throw error
    if (!Prog.has_value())
    {

        std::cerr << "INVALID PROGRAM" << std::endl;

        exit(EXIT_FAILURE);
    }

    // Generate will generate the al to asm code
    // And will create out.asm file
    {
        Generator generator(Prog.value());
        std::fstream file("out.asm", std::ios::out);
        file << generator.gen_prog();
    }

    // Compiling the asm file and linking
    // and generating the object file(machine code)
    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}