#include <cl/cl.h>

#include <string>
#include <fstream>
#include <iostream>

inline void cl_error_msg(std::string const& s,cl_int code)
{
	if(code != CL_SUCCESS)
		std::cout << s << " [" << code << "]" << std::endl;
}

void ocl_write_binary(cl_program program,std::string const & filename)
{
	cl_uint bin_size_c;
	clGetProgramInfo(program,CL_PROGRAM_BINARY_SIZES,0,0,&bin_size_c);
	size_t* bin_sizes = new size_t[bin_size_c];
	for(unsigned int i = 0;i < bin_size_c;++i)
		bin_sizes[i] = 0;
	clGetProgramInfo(program,CL_PROGRAM_BINARY_SIZES,bin_size_c,bin_sizes,NULL);

	std::cout << "binary sizes[";
	for(unsigned int i = 0;i < bin_size_c;++i)
		std::cout << " " << bin_sizes[i];
	std::cout << "]" << std::endl;

	size_t buffer_size = 0;
	char** buffer = new char*[bin_size_c];
	for(unsigned int i = 0;i < bin_size_c;++i,buffer_size += bin_sizes[i])
		buffer[i] = new char[bin_sizes[i]];

	std::ofstream ofs(filename,std::ios::binary);
	clGetProgramInfo(program,CL_PROGRAM_BINARIES,buffer_size,buffer,NULL);
	ofs.write(reinterpret_cast<char*>(&bin_size_c),sizeof(cl_uint));
	ofs.write(reinterpret_cast<char*>(bin_sizes),sizeof(size_t)*bin_size_c);
	for(unsigned int i = 0;i < bin_size_c;++i)
		ofs.write(buffer[i],bin_sizes[i]);

	for(unsigned int i = 0;i < bin_size_c;++i)
		delete [] buffer[i];
	delete [] buffer;
	delete [] bin_sizes;

	std::cout << "outputfile [" << filename << "]" << std::endl << std::endl;
	std::cout << "...compile....done" << std::endl;
}

void ocl_out_compile_error(cl_program program,cl_uint device_size,cl_device_id const * devices)
{
	cl_uint ret_size;
	for(unsigned int i = 0;i < device_size;++i)
	{
		clGetProgramBuildInfo(program,devices[i],CL_PROGRAM_BUILD_LOG,0,0,&ret_size);
		char *p = new char[ret_size];
		clGetProgramBuildInfo(program,devices[i],CL_PROGRAM_BUILD_LOG,ret_size,p,&ret_size);
		std::cout << "CL Build log..........." << std::endl;
		std::cout << p << std::endl;
		delete [] p;
	}
}

int main(int argc,char **argv)
{
	if(argc < 2)
	{
		std::cout << "no input file" << std::endl;
		return 1;
	}
	std::string filename(argv[1]);

	cl_int error;
	cl_platform_id platform;
	//init context
	error = ::clGetPlatformIDs(1, &platform, NULL);
	cl_error_msg("cl_platform_id can't create",error);
	
	cl_uint device_size;
	::clGetDeviceIDs(platform,CL_DEVICE_TYPE_GPU,0,0,&device_size);
	cl_device_id* devices = new cl_device_id[device_size];
	error = ::clGetDeviceIDs(platform,CL_DEVICE_TYPE_GPU,device_size,devices,NULL);
	cl_error_msg("cl_device can't create",error);
	
	cl_context ctx = ::clCreateContext(0,device_size,devices,NULL, NULL,&error);
	cl_error_msg("cl_context can't create",error);

	//build program
	std::ifstream ifs(filename);
	if( ifs.fail() )
	{
		std::cout << filename << " does not find" << std::endl;
		delete [] devices;
		return 1;
	}
	std::istreambuf_iterator<char> s_begin(ifs);
	std::istreambuf_iterator<char> s_end;
	std::string source(s_begin,s_end);
	const char *bfile = source.c_str();

	::cl_program program = clCreateProgramWithSource(ctx,1,&bfile,NULL,NULL);
	error = ::clBuildProgram(program,device_size,devices,NULL,NULL,NULL);
	
	if(error)//output error message
		ocl_out_compile_error(program,device_size,devices);
	else//output binary file
		ocl_write_binary(program,filename + ".bin");


	delete [] devices;
	::clUnloadCompiler();
	::clReleaseProgram(program);

	return 0;
}
