#include <iostream>
#include <fstream>
#include <string.h>
#include <time.h>

using namespace std;

//**************************************************************************
  // Initialize arrays A and B
//   for (int i = 0; i < N; i++)
//   {
//     A[i] = (float)(1.5 * (1 + (5 * i) % 7) / (1 + i % 5));
//     B[i] = (float)(2.0 * (2 + i % 5) / (1 + i % 7));
//   }


__global__ void inicializarVectores(float * A, float * B, const int size) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i < size){
        A[i] = (float)(1.5 * (1 + (5 * i) % 7) / (1 + i % 5));
        B[i] = (float)(2.0 * (2 + i % 5) / (1 + i % 7));
    }
}

__global__ void computaC_v1(float * A, float * B, float * C, const int k, const int bSize) {
    int istart = k * bSize, iend = istart + bSize; // k es el numero de bloque

    int i = (blockIdx.y * blockDim.y + threadIdx.y) + istart;
    int j = (blockIdx.x * blockDim.x + threadIdx.x) + istart;


    if (i < iend && j < iend){
        float a = A[j]*i;

        C[i] = 0.0;
        if ( (int)ceil(a) % 2 == 0 ){
            atomicAdd(&C[i], a + B[j]);
        }
        else{
            atomicAdd(&C[i], a - B[j]);
        }
    }

}


__global__ void computaC_compartida(float * A, float * B, float * C, const int i, const int istart, const int iend, const int size) {
    extern __shared__ float sdataA [];
    extern __shared__ float sdataB [];

    int tid = threadIdx.x;

    int j = (blockIdx.x * blockDim.x + threadIdx.x) + istart;

    if (j < size){
        sdataA[j] = A[j];
        sdataB[j] = B[j];
    }

    if (j < iend){
        float a = sDataA[j] * i;

        if ( (int)ceil(a) % 2 == 0 ){
            atomicAdd(&C[i], sdataA + sdataB[j]);
        }
        else{
            atomicAdd(&C[i], sdataA - sdataB[j]);

        }
        __syncthreads();
    }
}

__global__ void computaC_global(float * A, float * B, float * C, const int i, const int istart, const int iend) {
    int j = (blockIdx.x * blockDim.x + threadIdx.x) + istart;

    int index = i;

    if (j < iend){
        float a = A[j] * index;

        if ( (int)ceil(a) % 2 == 0 ){
            atomicAdd(&C[i], a + B[j]);
        }
        else{
            atomicAdd(&C[i], a - B[j]);
        }
    }

}


__global__ void calculaMaximoC(float * C, float * C_mx, float size) {
    extern __shared__ float sdata[];

    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    sdata[tid] = ((i < (size)) ? (C[i]) : 0.0f);
    __syncthreads();

    // Do reduction in shared memory
    for (int s=blockDim.x/2; s>0; s>>=1) {
        if (tid < s) {
            sdata[tid] = max(sdata[tid], sdata[tid+s]);
        }
        __syncthreads();
    }

    if (tid == 0) 
        C_mx[blockIdx.x] = sdata[0];
}


__global__ void calculaMaximoTotal(float * C, float * C_out, float size){
    extern __shared__ float sdata[];

    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    sdata[tid] = ((i < (size)) ? (C[i]) : 0.0f);
    __syncthreads();


    // Do reduction in shared memory
    for (int s=blockDim.x/2; s>0; s>>=1) {
        if (tid < s) {
            sdata[tid] = max(sdata[tid], sdata[tid+s]);
        }
        __syncthreads();
    }

    if (tid == 0) 
        C_out[blockIdx.x] = sdata[0];
}


__global__ void computaD(float * C, float * D, float size) {
    extern __shared__ float sdataD[];

    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    float valor = ((i < size) ? C[i] : 0.0f);
    sdataD[tid] = valor;
    __syncthreads();

    // Do reduction in shared memory
    for (int s=blockDim.x/2; s>0; s>>=1) {
        if (tid < s) {
            sdataD[tid] += sdataD[tid + s];
        }
        __syncthreads();
    }

    if (tid == 0) 
        D[blockIdx.x] = sdataD[0];
}




//**************************************************************************

//**************************************************************************
// ************  MAIN FUNCTION *********************************************
int main (int argc, char *argv[]) {

    double time, Tcpu, Tgpu;

    if (argc != 3) {
            cerr << "Sintaxis: " << argv[0] << " <./transformacion numbloques tambloque>" << endl;
                return(-1);
        }

    int nBlocks = atoi(argv[1]);
    int bSize = atoi(argv[2]);

    

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

    // Vectores
    float * d_A = NULL;
    float * d_B = NULL;
    float * d_C = NULL;
    float * d_D = NULL;
    float * d_C_mx = NULL;
    float * d_C_out = NULL;

    int N = nBlocks * bSize;
        int size = (nBlocks*bSize)*sizeof(float);
    int sizeD = nBlocks*sizeof(float);

    // cout << "size: " << size << endl;
    // cout << "sizeD: " << sizeD << endl;

    float * C_mx = new float[size];

    float * A = new float[nBlocks*bSize];
    float * B = new float[nBlocks*bSize];
    float * C = new float[nBlocks*bSize]; // antes size
    float * D = new float[nBlocks];

    float * C_out = new float[size];


    // Reserva de espacio
        err = cudaMalloc((void **) &d_A, size);
        if (err != cudaSuccess) {
                cerr << "ERROR MALLOC D_A" << endl;
        }

        err = cudaMalloc((void **) &d_B, size);
        if (err != cudaSuccess) {
                cerr << "ERROR MALLOC D_B" << endl;
        }

        err = cudaMalloc((void **) &d_C, size);
        if (err != cudaSuccess) {
                cerr << "ERROR MALLOC D_C" << endl;
        }

        err = cudaMalloc((void **) &d_D, sizeD);
        if (err != cudaSuccess) {
                cerr << "ERROR MALLOC D_D" << endl;
        }

    err = cudaMalloc((void **) &d_C_mx, size);
        if (err != cudaSuccess) {
                cerr << "ERROR MALLOC C_MX" << endl;
        }

    err = cudaMalloc((void **) &d_C_out, size);
        if (err != cudaSuccess) {
                cerr << "ERROR MALLOC C_OUT" << endl;
        }



    //**************************************************************************
        // GPU phase
        //**************************************************************************

    time=clock();


    inicializarVectores<<<nBlocks,bSize>>>(d_A, d_B, size);
    cudaDeviceSynchronize();
    err =cudaMemcpy(A, d_A, size, cudaMemcpyDeviceToHost);
        if (err != cudaSuccess) {
                cout << "ERROR CUDA MEM. COPY A" << endl;
        } 

    err =cudaMemcpy(B, d_B, size, cudaMemcpyDeviceToHost);
        if (err != cudaSuccess) {
                cout << "ERROR CUDA MEM. COPY B" << endl;
        } 

    int smemSizeC = nBlocks*bSize;
    for (int k = 0; k < nBlocks; k++){

        int istart = k * bSize; 
        int iend = istart + bSize;
        
        for (int i = istart; i < iend; i++){
            C[i] = 0.0;
            computaC_global<<<nBlocks, bSize>>>(d_A, d_B, d_C, i, istart, iend);
            cudaDeviceSynchronize();
            // computaC_compartida<<<nBlocks, bSize, smemSizeC>>>(d_A, d_B, d_C, i, istart, iend, size);
        }
    }
    cudaDeviceSynchronize();
    Tgpu=(clock()-time)/CLOCKS_PER_SEC;

    // Calculo del maximo
    int smemSize = bSize*sizeof(float);

    calculaMaximoC<<<nBlocks, bSize, smemSize>>>(d_C, d_C_mx, size);
    
    computaD<<<nBlocks, bSize, smemSize>>>(d_C, d_D, size);

    calculaMaximoTotal<<<nBlocks, bSize, smemSize>>>(d_C_mx, d_C_out, nBlocks);


    // Copia a host
    err =cudaMemcpy(C, d_C, size, cudaMemcpyDeviceToHost); 
        if (err != cudaSuccess) {
                cout << "ERROR CUDA MEM. COPY C" << endl;
        }

    err =cudaMemcpy(D, d_D, sizeD, cudaMemcpyDeviceToHost); 
        if (err != cudaSuccess) {
                cout << "ERROR CUDA MEM. COPY D" << endl;
        }

    err =cudaMemcpy(C_mx, d_C_mx, size, cudaMemcpyDeviceToHost); 
        if (err != cudaSuccess) {
                cout << "ERROR CUDA MEM. COPY C_MX" << endl;
        }

    err =cudaMemcpy(C_out, d_C_out, size, cudaMemcpyDeviceToHost); 
        if (err != cudaSuccess) {
                cout << "ERROR CUDA MEM. COPY C_MX" << endl;
        }

    // Imprimir matriz A
    // for (int i=0; i<4; i++){
    //     cout << "A["<<i<<"]: " << A[i] << endl;
    // }

    // Imprimir matriz B
    // for (int i=0; i<4; i++){
    //     cout << "B["<<i<<"]: " << B[i] << endl;
    // }

    cout << endl;

    // Imprimir matriz C
    // for (int i=0; i<N; i++){
    //     cout << "C["<<i<<"]: " << C[i] << endl;
    // } 
    
    // Imprimir matriz C_out
    // for (int i=0; i<N; i++){
    //     cout << "C_out["<<i<<"]: " << C_out[i] << endl;
    // }


    // Imprimir matriz C_mx
    // for (int i=0; i<nBlocks; i++){
    //     cout << "C_mx["<<i<<"]: " << C_mx[i] << endl;
    // }

    cout << endl;

    // Imprimir matriz D
    // for (int i = 0; i < nBlocks; i++){
    //     cout << "D["<<i<<"]: " << D[i] << endl;
    // }

    float d_max = C_out[0];

    // Imprimir maximo total
    cout << "Máximo --> C_out[0]: " << d_max << endl << endl;

/*
        Tgpu=(clock()-time)/CLOCKS_PER_SEC;
*/
        cout << "Time spent on GPU= " << Tgpu << endl << endl;

    //**************************************************************************
        // CPU phase
        //**************************************************************************

        time=clock();

    // ALGORITMO SECUENCIAL

    float *h_A, *h_B, *h_C, *h_D;

    //* Allocate arrays a, b and c on host*/
    h_A = new float[N];
    h_B = new float[N];
    h_C = new float[N];
    h_D = new float[nBlocks];
    float h_mx; // maximum of C

    // Initialize arrays A and B
    for (int i = 0; i < N; i++)
    {
        h_A[i] = (float)(1.5 * (1 + (5 * i) % 7) / (1 + i % 5));
        h_B[i] = (float)(2.0 * (2 + i % 5) / (1 + i % 7));
    }

    for (int k = 0; k < nBlocks; k++)
    {
        int istart = k * bSize;
        int iend = istart + bSize;
        for (int i = istart; i < iend; i++)
        {
            h_C[i] = 0.0;
            for (int j = istart; j < iend; j++)
            {
                float a = h_A[j] * i;
                if ((int)ceil(a) % 2 == 0)
                h_C[i] += a + h_B[j];
                else
                h_C[i] += a - h_B[j];
            }
        }
    }


    // Compute mx
    h_mx = h_C[0];
    for (int i = 1; i < N; i++)
    {
        h_mx = max(h_C[i], h_mx);
    }

    // Compute d[K]
    for (int k = 0; k < nBlocks; k++)
    {
        int istart = k * bSize;
        int iend = istart + bSize;
        h_D[k] = 0.0;
        for (int i = istart; i < iend; i++)
        {
            h_D[k] += h_C[i];
        }
    }


  Tcpu=(clock()-time)/CLOCKS_PER_SEC;
  cout << "Time spent on CPU= " << Tcpu << endl << endl;
  cout<<"....................................................."<<endl<<endl;

  cout << "Speedup TCPU/TGPU= " << Tcpu / Tgpu << endl;
  cout<<"....................................................."<<endl<<endl;

  
  bool errors=false;
  float epsilon = 0.0000000000000001f; //Umbral para comprobar fallo de coma flotante
//   float epsilon = 0.1f; //Umbral para comprobar fallo de coma flotante

  // Error Checking (CPU vs. GPU)

    // for (int i=0; i<N; i++){
    //     cout << "h_C["<<i<<"]: " << h_C[i] << endl;
    // }
    for (int i=0; i<N; i++){
        if (abs(h_A[i] - A[i]) >= epsilon){
                cout << "ERROR A (i: " << i << ") " << h_A[i] << "!=" << A[i] << endl;
            //cout << "Error: A no coincide" << endl;
            //cout << "fallo en i: " << i << " con valor A["<<i<<"]: " << A[i];
            //cout << " y h_A["<<i<<"]" << h_A[i] << endl;
            errors = true;
        }
    }
    cout << endl;
    for (int i=0; i<N; i++){
        if (abs(h_B[i] - B[i]) >= epsilon){
                cout << "ERROR B (i: " << i << ") " << h_B[i] << "!=" << B[i] << endl;
            //cout << "Error: B no coinciden" << endl;
            //cout << "fallo en i: " << i << " con valor B["<<i<<"]: " << B[i];
            //cout << " y h_B["<<i<<"]" << h_B[i] << endl;
            errors = true;
        }
    }
    cout << endl;
    for (int i=0; i<N; i++){
        if (abs(h_C[i] - C[i]) >= epsilon){
            cout << "ERROR C(i: " << i << ") " << h_C[i] << "!=" << C[i] << endl;

            //cout << "C no coincide" << endl;
            //cout << "fallo en i: " << i << " con valor C["<<i<<"]: " << C[i];
            //cout << " y h_C["<<i<<"]" << h_C[i] << endl;
            errors = true;
        }
    }

    cout << endl;
    
    for (int i=0; i < nBlocks; i++){
        if(abs(h_D[i] - D[i]) >= epsilon){
            cout << "ERROR D(i: " << i << ") " << h_D[i] << "!=" << D[i] << endl;
            //cout << "Error: D no coincide con valor D["<<i<<"]: " << D[i];
            //cout << " y h_D["<<i<<"]: " << h_D[i] << endl; 
            errors = true;
            // break;
        }
    }
    cout << endl;
    if(d_max != h_mx){
        cout << "ERROR MAXIMO(" << d_max << "!=" << h_mx << ")"  << endl;
        errors = true;
    }


  if (!errors){ 
    cout<<"....................................................."<<endl;
        cout<< "WELL DONE!!! No errors found ............................"<<endl;
        cout<<"....................................................."<<endl<<endl;
  }

    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);    
    cudaFree(d_D);   
    cudaFree(d_C_mx);
    cudaFree(d_C_out);

    cudaFree(A);
    cudaFree(B);
    cudaFree(C);    
    cudaFree(D);    
    cudaFree(C_mx);
    cudaFree(C_out);

    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
    delete[] h_D;
}
