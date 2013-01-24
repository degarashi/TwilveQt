#include <iostream>
#include <fstream>
#include <cstring>
#include <stdexcept>

int main(int argc, char **argv) {
	if(argc == 4) {
		try {
			bool bEnc;
			if(!std::strcmp(argv[1], "-e")) {
				bEnc = true;
			} else if(!std::strcmp(argv[1], "-d")) {
				bEnc = false;
			} else
				throw std::runtime_error(std::string("unknown option ") + argv[1]);
			
			std::ifstream fs(argv[2], std::ios::binary);
			if(!fs.is_open())
				throw std::runtime_error("cannot open input file");
			
			fs.seekg(0, std::ios::end);
			int len = static_cast<int>(fs.tellg());
			fs.seekg(0, std::ios::beg);
			
			char tc[256];
			fs.read(tc, sizeof(tc));
			if(!fs.eof())
				throw std::runtime_error("input buffer is too large");
			
			// 今はエンコードとデコードが同じだけど、そのうち変えるかもしれない
			if(bEnc) {
				// エンコード処理
				for(int i=0 ; i<len ; i++)
					tc[i] ^= (i*0xab)^0xc4;
			} else {
				// デコード処理
				for(int i=0 ; i<len ; i++)
					tc[i] ^= (i*0xab)^0xc4;
			}
			// 結果を書き込み
			std::ofstream ofs(argv[3], std::ios::binary);
			if(!ofs.is_open())
				throw std::runtime_error("connot open output file");
			ofs.write(tc, len);
		} catch (const std::exception& e) {
			std::cout << "error:" << std::endl << e.what() << std::endl;
		}
	} else {
		std::cout << "Usage: enc [Flag (-e or -d)] [InputFile] [OutputFile]" << std::endl;
	}
    return 0;
}
