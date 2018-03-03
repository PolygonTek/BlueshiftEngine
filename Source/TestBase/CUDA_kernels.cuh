void CUDA_Add(float *dst, const float *src0, const float *src1, const int count);
void CUDA_Sub(float *dst, const float *src0, const float *src1, const int count);
void CUDA_Mul(float *dst, const float *src0, const float *src1, const int count);
void CUDA_Div(float *dst, const float *src0, const float *src1, const int count);
void CUDA_Sum(float *dst, const float *src, const int count);
void CUDA_MatrixMultiply(float *dst, const float *src0, const float *src1, const int n);