#include "mpi.h"
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>

const static int ARRAY_SIZE = 130000;
using Lines = char[ARRAY_SIZE][16];

// To remove punctuations
struct letter_only: std::ctype<char> 
{
    letter_only(): std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const* get_table()
    {
        static std::vector<std::ctype_base::mask> 
            rc(std::ctype<char>::table_size,std::ctype_base::space);

        std::fill(&rc['A'], &rc['z'+1], std::ctype_base::alpha);
        return &rc[0];
    }
};

void DoOutput(std::string word, int result)
{
    std::cout << "Word Frequency: " << word << " -> " << result << std::endl;
}

//***************** Add your functions here *********************

int main(int argc, char* argv[])
{
    MPI_Status status;
    int processId;
    int num_processes;
    int *to_return = NULL;
    double start_time, end_time;
    int localsum, start_array, length, returning, chunksize,sendsum;

 
    // Setup MPI
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &processId);
    MPI_Comm_size( MPI_COMM_WORLD, &num_processes);
 
    // Three arguments: <input file> <search word> <part B1 or part B2 to execute>
    if(argc != 4)
    {
        if(processId == 0)
        {
            std::cout << "ERROR: Incorrect number of arguments. Format is: <filename> <word> <b1/b2>" << std::endl;
        }
        MPI_Finalize();
        return 0;
    }
	std::string word = argv[2];
 
    Lines lines;
	// Read the input file and put words into char array(lines)
    if (processId == 0) {
        std::ifstream file;
		file.imbue(std::locale(std::locale(), new letter_only()));
		file.open(argv[1]);
		std::string workString;
		int i = 0;
		while(file >> workString){
			memset(lines[i], '\0', 16);
			memcpy(lines[i++], workString.c_str(), workString.length());
		}
        length = i;
    }
	
	//***************** Add code as per your requirement below ***************** 
	
	start_time=MPI_Wtime();

    

	if( std::string(argv[3]) == "b1" )
	{

        if(processId == 0)
        {
            chunksize = length/num_processes;
            for(int an_id = 1; an_id < num_processes; an_id++)
            {
                chunksize = length/num_processes;
                start_array = an_id * chunksize;
                //std:: cout << "array start on " << start_array << std::endl;
 
                MPI_Send(&chunksize, 1, MPI_INT, an_id, 1, MPI_COMM_WORLD);
                MPI_Send(lines+start_array, chunksize, MPI_LONG_DOUBLE, an_id, 2, MPI_COMM_WORLD);
            }

        }
        else{

            MPI_Recv(&chunksize, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
            MPI_Recv(&lines, chunksize, MPI_LONG_DOUBLE, 0, 2, MPI_COMM_WORLD, &status);
           

        }


        for(int i = 0; i < chunksize; i++)
        {
            if(lines[i] == std::string(argv[2]))
                localsum +=1;
             

        }
        //std:: cout << "Processor " << processId << " found " << localsum <<" of " <<std::string(argv[2]) << std::endl;
        
        MPI_Reduce(&localsum, &returning, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        to_return = &returning;

	} 
    else
    {
        if(processId == 0)
        {
            chunksize = length/num_processes;
            for(int an_id = 1; an_id < num_processes; an_id++)
            {
                chunksize = length/num_processes;
                start_array = an_id * chunksize;
                //std:: cout << "array start on " << start_array << std::endl;
 
                MPI_Send(&chunksize, 1, MPI_INT, an_id, 1, MPI_COMM_WORLD);
                MPI_Send(lines+start_array, chunksize, MPI_LONG_DOUBLE, an_id, 2, MPI_COMM_WORLD);
            }

        }
        else{

            MPI_Recv(&chunksize, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
            MPI_Recv(&lines, chunksize, MPI_LONG_DOUBLE, 0, 2, MPI_COMM_WORLD, &status);
           

        }


        for(int i = 0; i < chunksize; i++)
        {
            if(lines[i] == std::string(argv[2]))
                localsum +=1;

        }
        int sendsum = localsum;
        int t = false;
    
        if(processId == 0)
        {
            MPI_Send(&sendsum, 1, MPI_INT, processId+1, 3, MPI_COMM_WORLD);
        }
        else{
            MPI_Recv(&sendsum, 1, MPI_INT, processId-1, 3, MPI_COMM_WORLD, &status);
            //std::cout << "Processor "<< processId<<" Recieved " << sendsum << " add and passing on :" << sendsum+localsum << std::endl;
            localsum += sendsum;
            sendsum = localsum;
            if(processId < num_processes){
               // std::cout << "sending" << std::endl;
                MPI_Send(&sendsum, 1, MPI_INT, (processId+1)%num_processes, 3, MPI_COMM_WORLD);
                }
        }
        if(processId == 0)
         {   
        
         MPI_Recv(&sendsum, 1, MPI_INT, num_processes-1, 3, MPI_COMM_WORLD, &status);
         to_return = &sendsum;
    
         }
        
    


    }
	
    if(processId == 0)
    {   
    
        // Output the search word's frequency here
		DoOutput(std::string(argv[2]), *to_return );
		end_time=MPI_Wtime();
        std::cout << "Time: " << ((double)end_time-start_time) << std::endl;
    }
 
    MPI_Finalize();
 
    return 0;
}