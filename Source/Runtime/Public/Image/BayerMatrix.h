// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

BE_NAMESPACE_BEGIN

template <int MatrixSize>
class BayerMatrix {
public:
    static_assert(MatrixSize == 2 || MatrixSize == 4 || MatrixSize == 8 || MatrixSize == 16, "Bayer matrix size must be 2, 4, 8, or 16");

    static constexpr int    Size = MatrixSize;
    static constexpr int    CellCount = MatrixSize * MatrixSize;

                            /// 좌표를 MatrixSize 주기로 반복하여 해당 셀의 Bayer threshold rank 를 [0, CellCount) 범위로 리턴한다.
    static constexpr byte   GetRank(int x, int y);

                            /// (rank + 0.5) / CellCount 로 정규화한 (0, 1) 범위의 threshold 를 리턴한다.
    static constexpr float  GetThreshold(int x, int y);

                            /// (threshold - 0.5) * quantizationStep 으로 0 중심의 dither offset 을 리턴한다.
    static constexpr float  GetDitherOffset(int x, int y, float quantizationStep);

private:
    struct MatrixData {
        constexpr MatrixData();

        byte values[CellCount];
    };

    inline static constexpr MatrixData matrixData {};
};

template <int MatrixSize>
constexpr BayerMatrix<MatrixSize>::MatrixData::MatrixData() : values {} {
    // 기존 rank 를 4 배하고 2x2 Bayer 순서인 0, 2, 3, 1 을 더해 다음 크기로 확장한다.
    for (int submatrixSize = 1; submatrixSize < MatrixSize; submatrixSize <<= 1) {
        for (int y = 0; y < submatrixSize; ++y) {
            for (int x = 0; x < submatrixSize; ++x) {
                const int index = y * MatrixSize + x;
                const int scaledRank = static_cast<int>(values[index]) * 4;

                values[index] = static_cast<byte>(scaledRank);
                values[index + submatrixSize] = static_cast<byte>(scaledRank + 2);
                values[index + submatrixSize * MatrixSize] = static_cast<byte>(scaledRank + 3);
                values[index + submatrixSize * MatrixSize + submatrixSize] = static_cast<byte>(scaledRank + 1);
            }
        }
    }
}

template <int MatrixSize>
constexpr byte BayerMatrix<MatrixSize>::GetRank(int x, int y) {
    const int index = (y & (MatrixSize - 1)) * MatrixSize + (x & (MatrixSize - 1));
    return matrixData.values[index];
}

template <int MatrixSize>
constexpr float BayerMatrix<MatrixSize>::GetThreshold(int x, int y) {
    return (static_cast<float>(GetRank(x, y)) + 0.5f) * (1.0f / static_cast<float>(CellCount));
}

template <int MatrixSize>
constexpr float BayerMatrix<MatrixSize>::GetDitherOffset(int x, int y, float quantizationStep) {
    return (GetThreshold(x, y) - 0.5f) * quantizationStep;
}

BE_NAMESPACE_END
