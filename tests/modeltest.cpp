#include "model.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	std::cout << "Model Test \n";

	Model model(4, 4, 4, 0.1);

	model.debugToLog();

	Eigen::Vector3f c(0.0f, 0.0f, 0.0f);
	model.generateSphere(0.1, c);

	system("pause");
	return 0;
}