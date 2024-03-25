#include <iostream>
#include <fstream>
#include <string.h>
#include <time.h>
#include "Graph.h"

using namespace std;

#define blocksize 64

//**************************************************************************
// FLOYD 1D
// __global__ void floyd_kernel(int * M, const int nverts, const int k) {
//     int ij = threadIdx.x + blockDim.x * blockIdx.x;
//     int i= ij / nverts;
//     int j= ij - i * nverts;
//     if (i<nverts && j< nverts) {
//     int Mij = M[ij];
//     if (i != j && i != k && j != k) {
// 	int Mikj = M[i * nverts + k] + M[k * nverts + j];
//     Mij = (Mij > Mikj) ? Mikj : Mij;
//     M[ij] = Mij;}
//   }
// }

// FLOYD 2D
__global__ void floyd_kernel(int * M, const int nverts, const int k) {
	int j = blockIdx.x * blockDim.x + threadIdx.x;
    int i = blockIdx.y * blockDim.y + threadIdx.y;

    if (i < nverts && j < nverts) {
		int ij = i * nverts + j;
		int Mij = M[ij];

		if (i != j && i != k && j != k) {
			int Mikj = M[i * nverts + k] + M[k * nverts + j];
			Mij = (Mij > Mikj) ? Mikj : Mij;
			M[ij] = Mij;
		}
  	}
}

__global__ void reduceMediaAritmetica(int * M, long int * M_out, const int nverts) {
	// Shared memory vector to store the data
	extern __shared__ float sdata[];

    // Compute global index i and j to access the matrix (vector) M
    int tid = threadIdx.x;
	int j = blockIdx.x * blockDim.x + threadIdx.x;
    int i = blockIdx.y * blockDim.y + threadIdx.y;

    int ij = i * nverts + j;
    float Mij = 0.0f; // Initialize Mij with a float value

    // Check bounds before accessing M
    if (i < nverts && j < nverts) {
        Mij = static_cast<float>(M[ij]); // Convert Mij to float
    }

    // Load data into shared memory
    sdata[tid] = Mij;
    __syncthreads();

	// Do reduction in shared memory
	for(int s = blockDim.x/2; s > 0; s >>= 1){
		if (tid < s)
			sdata[tid] += sdata[tid + s]; 	
		__syncthreads();
	}

	// Write result for this block to global memory
	if (tid == 0) 
		// M_out[blockIdx.x] = sdata[0];
		M_out[j * gridDim.x + blockIdx.x] = sdata[0];

	// M_out[0] = sdata[0];

}


// __global__ void media_aritmetica(int * M, const int nverts, const int k) {
// 	int j = blockIdx.x * blockDim.x + threadIdx.x;
//     int i = blockIdx.y * blockDim.y + threadIdx.y;

//     if (i < j && j < nverts) {
// 		int ij = i * nverts + j;
// 		int Mij = M[ij];

// 		if(M[ij] != 0){
// 			sumaCaminos += M[ij];
// 			totalCaminos++;
// 		}
// 		else{
// 			break;
// 		}

//   	}
// }

// numero de elementos totales es N*N-N y luego entre 2

		// for (int i=0; i < nverts; i++){
		// 	for (int j=0; j < nverts; j++){
		// 		if(M[ij] != 0){
		// 			sumaCaminos += M[ij];
		// 			totalCaminos++;
		// 		}
		// 		else{
		// 			break;
		// 		}
		// 	}
		// }
//**************************************************************************

//**************************************************************************
// ************  MAIN FUNCTION *********************************************
int main (int argc, char *argv[]) {

    double time, Tcpu, Tgpu;

    if (argc != 2) {
	    cerr << "Sintaxis: " << argv[0] << " <archivo de grafo>" << endl;
		return(-1);
	}	

    //Get GPU information
    int num_devices,devID;
    cudaDeviceProp props;
    cudaError_t err;

	err=cudaGetDeviceCount(&num_devices);
	if (err == cudaSuccess) { 
	    cout <<endl<< num_devices <<" CUDA-enabled  GPUs detected in this computer system"<<endl<<endl;
		cout<<"....................................................."<<endl<<endl;}	
	else 
	    { cerr << "ERROR detecting CUDA devices......" << endl; exit(-1);}
	    
	for (int i = 0; i < num_devices; i++) {
	    devID=i;
	    err = cudaGetDeviceProperties(&props, devID);
        cout<<"Device "<<devID<<": "<< props.name <<" with Compute Capability: "<<props.major<<"."<<props.minor<<endl<<endl;
        if (err != cudaSuccess) {
		  cerr << "ERROR getting CUDA devices" << endl;
	    }


	}
	devID = 0;    
        cout<<"Using Device "<<devID<<endl;
        cout<<"....................................................."<<endl<<endl;

	err = cudaSetDevice(devID); 
    if(err != cudaSuccess) {
		cerr << "ERROR setting CUDA device" <<devID<< endl;
	}

	// Declaration of the Graph object
	Graph G;
	
	// Read the Graph
	G.lee(argv[1]);

	//cout << "The input Graph:"<<endl;
	//G.imprime();
	const int nverts = G.vertices;
	const int niters = nverts;
	const int nverts2 = nverts * nverts;

	int *c_Out_M = new int[nverts2];
	int size = nverts2*sizeof(int);
	int * d_In_M = NULL;

	////
	long int * d_Out_M = NULL;
	long int *c_Out_M_media = new long int[nverts2];

	////

	err = cudaMalloc((void **) &d_In_M, size);

	if (err != cudaSuccess) {
		cerr << "ERROR MALLOC D_IN" << endl;
	}

	err = cudaMalloc((void **) &d_Out_M, size);
	if (err != cudaSuccess) {
		cerr << "ERROR MALLOC D_OUT" << endl;
	}


    // Get the integer 2D array for the dense graph
	int *A = G.Get_Matrix();

    //**************************************************************************
	// GPU phase
	//**************************************************************************
	
    time=clock();

	err = cudaMemcpy(d_In_M, A, size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		cout << "ERROR CUDA MEM. COPY" << endl;
	} 

    // Main Loop
	for(int k = 0; k < niters; k++) {
		//printf("CUDA kernel launch \n");
		
		// Unidimensional
	 	// int threadsPerBlock = blocksize;
	 	// int blocksPerGrid = (nverts2 + threadsPerBlock - 1) / threadsPerBlock;


		// Bidimensional
		int a = sqrt(blocksize);
		dim3 threadsPerBlock (a, a);
		dim3 blocksPerGrid( ceil ((float)(nverts)/threadsPerBlock.x), ceil ((float)(nverts)/threadsPerBlock.y) );
        
		// Kernel Launch
	    floyd_kernel<<<blocksPerGrid,threadsPerBlock >>>(d_In_M, nverts, k);
	    err = cudaGetLastError();

	    if (err != cudaSuccess) {
	  	    fprintf(stderr, "Failed to launch kernel! ERROR= %d\n",err);
	  	    exit(EXIT_FAILURE);
		}
	}


	// Reduction
	int a = sqrt(blocksize);
	dim3 threadsPerBlock (a, a);
	// dim3 numBlocks( ceil((float)(nverts)/threadsPerBlock.x), 1 );
	dim3 numBlocks(ceil((float)(nverts) / threadsPerBlock.x), ceil((float)(nverts) / threadsPerBlock.y));
	int smemSize = threadsPerBlock.x*threadsPerBlock.y*sizeof(float);
	cout << "antes reduce" << endl;
	reduceMediaAritmetica<<<numBlocks, threadsPerBlock, smemSize>>>(d_In_M, d_Out_M, nverts);
	cout << "despues reduce" << endl;




	err =cudaMemcpy(c_Out_M, d_In_M, size, cudaMemcpyDeviceToHost); // ejercicio 1-1
	err =cudaMemcpy(c_Out_M_media, d_Out_M, size, cudaMemcpyDeviceToHost); // ejercicio 1-2
	if (err != cudaSuccess) {
		cout << "ERROR CUDA MEM. COPY" << endl;
	} 

	// Imprimir floyd 
	// for(int i=0; i<nverts2; i++){
	// 		cout << "C[i]=" << c_Out_M[i] << endl;
	// }


	cout << "c_out_media[0]: " << c_Out_M_media[0] << endl;
	cout << "nverts:" << nverts << endl;
	cout << "n*n-n=" << (nverts*nverts-nverts) << endl;
	float media = 0;
	media = (float)( (c_Out_M_media[0] * 1.0) / ((nverts*nverts-nverts) * 1.0) ); // suma final
	// cout << "revivo" << endl;

	cout << "La media aritmetica es: " << media << endl;


	Tgpu=(clock()-time)/CLOCKS_PER_SEC;
	
	cout << "Time spent on GPU= " << Tgpu << endl << endl;

    //**************************************************************************
	// CPU phase
	//**************************************************************************

	time=clock();

	// BUCLE PPAL DEL ALGORITMO
	int inj, in, kn;
	for(int k = 0; k < niters; k++) {
          kn = k * nverts;
	  for(int i=0;i<nverts;i++) {
			in = i * nverts;
			for(int j = 0; j < nverts; j++)
	       			if (i!=j && i!=k && j!=k){
			 	    inj = in + j;
			 	    A[inj] = min(A[in+k] + A[kn+j], A[inj]);
	       }
	   }
	}
  
  Tcpu=(clock()-time)/CLOCKS_PER_SEC;
  cout << "Time spent on CPU= " << Tcpu << endl << endl;
  cout<<"....................................................."<<endl<<endl;

  cout << "Speedup TCPU/TGPU= " << Tcpu / Tgpu << endl;
  cout<<"....................................................."<<endl<<endl;

  
  bool errors=false;
  // Error Checking (CPU vs. GPU)
  for(int i = 0; i < nverts; i++)
    for(int j = 0; j < nverts; j++)
       if (abs(c_Out_M[i*nverts+j] - G.arista(i,j)) > 0)
         {cout << "Error (" << i << "," << j << ")   " << c_Out_M[i*nverts+j] << "..." << G.arista(i,j) << endl;
		  errors=true;
		 }


  if (!errors){ 
    cout<<"....................................................."<<endl;
	cout<< "WELL DONE!!! No errors found ............................"<<endl;
	cout<<"....................................................."<<endl<<endl;

  }
  cudaFree(d_In_M);
	cudaFree(d_Out_M);
}


