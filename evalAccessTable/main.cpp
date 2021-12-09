//
// Created by dev on 26.10.21.
//

#include "evalAccessTable.hpp"

int main(int argc, char *argv[]) {
    if(argc == 2){
        EvalAccessTable eval(argv[1], "./out/");
        return 0;
    }else if(argc == 3){
        EvalAccessTable eval(argv[1], argv[2]);
        return 0;
    }else {
        std::cout << "Usage: " << argv[0] << "tracefile [outdir]" <<std::endl;
	return 1;
    }
}
