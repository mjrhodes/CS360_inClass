#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
main()
{
	int pid = fork();
	std::cout << "Fork Returned " << pid << std::endl;
	if(pid == 0) {
		std::cout << "Child about to exec " << std::endl;
		execl("/bin/ls","/bin/ls",(char *)0);
		std::cout << "Child after exec" << std::endl;
	} else {
		int status;
		wait(&status);
		std::cout << "Parent after wait" << std::endl;
	}
}
