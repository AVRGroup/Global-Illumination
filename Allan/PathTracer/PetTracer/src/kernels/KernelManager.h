#pragma once

#include <CLW.h>

#include <map>
#include <vector>
#include <string>

namespace PetTracer
{

	class KernelManager
	{
	public:
		KernelManager();
		~KernelManager();

		void OpenProgram( std::string& path );

		CLWKernel& GetKernel( std::string& kernelName );

	private:
		std::vector<CLWProgram> mPrograms;


	};

}