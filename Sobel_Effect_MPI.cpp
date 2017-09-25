#include "mpi.h"
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

//***************** Add/Change the functions(including processImage) here ********************* 

// NOTE: Some arrays are linearized in the skeleton below to ease the MPI Data Communication 
// i.e. A[x][y] become A[x*total_number_of_columns + y]
int* processImage(int* inputImage, int processId, int num_processes, int image_height, int image_width){
     int x, y, sum, sumx, sumy;
     int GX[3][3], GY[3][3];
     /* 3x3 Sobel masks. */
     GX[0][0] = -1; GX[0][1] = 0; GX[0][2] = 1;
     GX[1][0] = -2; GX[1][1] = 0; GX[1][2] = 2;
     GX[2][0] = -1; GX[2][1] = 0; GX[2][2] = 1;
     
     GY[0][0] =  1; GY[0][1] =  2; GY[0][2] =  1;
     GY[1][0] =  0; GY[1][1] =  0; GY[1][2] =  0;
     GY[2][0] = -1; GY[2][1] = -2; GY[2][2] = -1;
	
	//chunkSize is the number of rows to be computed by this process
	int chunkSize, start_of_chunk, end_of_chunk;
    int* partialOutputImage;
    // gets the chunksize
    chunkSize = image_height/num_processes;
    partialOutputImage = new int[chunkSize*image_width];
 
    //std::cout << "\nProcessor " << processId << ": im about to convert with chunksize = "<< chunkSize*image_width << std:: endl;
    //std::cout << inputImage << " " << processId << " " << num_processes << " " << image_height << " " << image_width << std::endl;
	//needs to know the roll 
	for( x = 0; x < chunkSize-1; x++ ){
		 for( y = 0; y < image_width; y++ ){
			 sumx = 0;
			 sumy = 0;
			
			//Change boundary cases as required
			 if( x==0 || x==(image_height-1) || y==0 || y==(image_width-1))
				 sum = 0;
			 else{

				 for(int i=-1; i<=1; i++)  {
					 for(int j=-1; j<=1; j++){
						 sumx += (inputImage[(x+i)*image_width+ (y+j)] * GX[i+1][j+1]);
					 }
				 }

				 for(int i=-1; i<=1; i++)  {
					 for(int j=-1; j<=1; j++){
						 sumy += (inputImage[(x+i)*image_width+ (y+j)] * GY[i+1][j+1]);
					 }
				 }

				 sum = (abs(sumx) + abs(sumy));
			 }
			 if (sum < 0)
			 	partialOutputImage[x*image_width + y] = 0;
			 else if( sum > 200)
			 	partialOutputImage[x*image_width + y]  = 255;
		 }
	}
	return partialOutputImage;
}

int main(int argc, char* argv[])
{
	int processId, num_processes, image_height, image_width, image_maxShades;
	int *inputImage, *outputImage, *partialOutputImage;
	int *localdata, average_chunk_size;
	MPI_Status Status;
	
	int ierr; 
	// Setup MPI
    ierr = MPI_Init( &argc, &argv );
    ierr = MPI_Comm_rank( MPI_COMM_WORLD, &processId);
    ierr = MPI_Comm_size( MPI_COMM_WORLD, &num_processes);

    
	
    if(argc != 3)
    {
		if(processId == 0)
			std::cout << "ERROR: Incorrect number of arguments. Format is: <Input image filename> <Output image filename>" << std::endl;
		MPI_Finalize();
        return 0;
    }
	
	if(processId == 0)
	{
		std::ifstream file(argv[1]);
		if(!file.is_open())
		{
			std::cout << "ERROR: Could not open file " << argv[1] << std::endl;
			MPI_Finalize();
			return 0;
		}

		std::cout << "Detect edges in " << argv[1] << " using " << num_processes << " processes\n" << std::endl;

		std::string workString;
		/* Remove comments '#' and check image format */ 
		while(std::getline(file,workString))
		{
			if( workString.at(0) != '#' ){
				if( workString.at(1) != '2' ){
					std::cout << "Input image is not a valid PGM image" << std::endl;
					return 0;
				} else {
					break;
				}       
			} else {
				continue;
			}
		}
		/* Check image size */ 
		while(std::getline(file,workString))
		{
			if( workString.at(0) != '#' ){
				std::stringstream stream(workString);
				int n;
				stream >> n;
				image_width = n;
				stream >> n;
				image_height = n;
				break;
			} else {
				continue;
			}
		}
		inputImage = new int[image_height*image_width];

		/* Check image max shades */ 
		while(std::getline(file,workString))
		{
			if( workString.at(0) != '#' ){
				std::stringstream stream(workString);
				stream >> image_maxShades;
				break;
			} else {
				continue;
			}
		}
		/* Fill input image matrix */ 
		int pixel_val;
		for( int i = 0; i < image_height; i++ )
		{
			if( std::getline(file,workString) && workString.at(0) != '#' ){
				std::stringstream stream(workString);
				for( int j = 0; j < image_width; j++ ){
					if( !stream )
						break;
					stream >> pixel_val;
					inputImage[(i*image_width)+j] = pixel_val;
				}
			} else {
				continue;
			}
		}
	} // Done with reading image using process 0
	
	//***************** Add code as per your requirement below ********************* 
	
	//needs to distribute the work through the master process since it has all information
	if(processId == 0){
	//set up the outputImage array
		outputImage = new int[image_height*image_width];
		
		average_chunk_size = image_width*image_height/num_processes; 

	   	int start_of_chunk = average_chunk_size * processId ;
		//needs to send the formation from main process to other
		for(int an_id = 1; an_id < num_processes; an_id++)
		{
			ierr = MPI_Send(&image_height, 1, MPI_INT, an_id, 2001, MPI_COMM_WORLD);
			ierr = MPI_Send(&image_width, 1, MPI_INT, an_id, 2001, MPI_COMM_WORLD);
			//ierr = MPI_Send(&inputImage, total_chunk, MPI_INT, an_id, 2001, MPI_COMM_WORLD);
		}
	}
	else{

		ierr = MPI_Recv(&image_height, 1, MPI_INT, 0, 2001, MPI_COMM_WORLD, &Status);
		ierr = MPI_Recv(&image_width,1, MPI_INT, 0, 2001, MPI_COMM_WORLD, &Status);
		//ierr = MPI_Recv(&inputImage, , MPI_INT, 0, 2001, MPI_COMM_WORLD, &Status);


	}
	average_chunk_size = image_width*image_height/num_processes ;
	
	
	localdata = new int[average_chunk_size];
	
	MPI_Scatter(inputImage, average_chunk_size, MPI_INT, localdata, average_chunk_size ,MPI_INT, 0 , MPI_COMM_WORLD);

	//std::cout <<" Process " << processId << " has width = " << image_width <<" and height = " << image_height << std::endl;
	//make process 0 do work
	partialOutputImage = processImage(localdata, processId , num_processes , image_height,  image_width);
	
	
	MPI_Gather(partialOutputImage, average_chunk_size, MPI_INT, outputImage, average_chunk_size, MPI_INT, 0, MPI_COMM_WORLD);
	



	//collects all the data
	//MPI_Gather(&localdata, 1, MPI_INT, outputImage,  1, MPI_INT, 0, MPI_COMM_WORLD);

	if(processId == 0)
	{
		/* Start writing output to your file  */
		std::ofstream ofile(argv[2]);
	
		if( ofile.is_open() )
		{
			ofile << "P2" << "\n" << image_width << " " << image_height << "\n" << image_maxShades << "\n";
			for( int i = 0; i < image_height; i++ )
			{
				for( int j = 0; j < image_width; j++ ){
					ofile << outputImage[(i*image_width)+j] << " ";
				}
				ofile << "\n";
			}
		} else {
			std::cout << "ERROR: Could not open output file " << argv[2] << std::endl;
			return 0;
		}	
	}  
	 
    ierr = MPI_Finalize();
    return 0;
}




