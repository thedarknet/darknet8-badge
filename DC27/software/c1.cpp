#include <iostream>
#include <string>

//uint8_t c[] = { 'C', 'L', 'O', 'U', 'D', ' ', '9'};
uint8_t t[] = {  3,  12, 15, 20, 4, 10, 9};
uint8_t p[] = { 64, 88, 64, 105, 64, 42, 48};

bool compare(uint8_t l, int i) {
	uint8_t tt;
	if(i&1) {
		tt = l+t[i];
	} else {
		tt = l-t[i];
	}
	return tt==p[i];
}


bool validatePassword(std::string &pw) {
	for(int i = 0;i<pw.length();++i) {
		char up = pw[i];
		if(!compare(uint8_t(up), i)) {
			return false;
		}
	}
	return true;
}

int main(int arc, char *argv[]) {
/*
	for(int i=0;i<sizeof(c);++i) {
		if(i&1) {
			std::cout << c[i]+t[i] << ", ";
		} else {
			std::cout << c[i]-t[i] << ", ";
		}
	}
*/
	std::cout << "Password: " << std::endl;
	std::string passwd;
	std::getline(std::cin,passwd);
	if(validatePassword(passwd)) {
		std::cout << "Enter the password into the daemon" << std::endl;
	} else {
		std::cout << "incorrect...." << std::endl;
	}
	return 0;
}
