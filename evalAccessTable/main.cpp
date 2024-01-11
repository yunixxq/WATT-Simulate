//
// Created by dev on 26.10.21.
//

#include "evalAccessTable.hpp"

int main(int argc, char *argv[]) {
    bool test = false;
    bool benchmark = true;
    if(argc == 2){
        EvalAccessTable eval(argv[1], "./out/", true, test, benchmark);
        return 0;
    }else if(argc == 3){
        EvalAccessTable eval(argv[1], argv[2], true, test, benchmark);
        return 0;
    }else {
        std::cout << "Usage: " << argv[0] << "tracefile [outdir]" <<std::endl;
	return 1;
    }
}
