/*
    vectorbox.h -- NxM grid of textboxes to edit matrix values.

    This widget was created by Ashwin Chetty.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#pragma once

#include <nanogui/layout.h>
#include <nanogui/textbox.h>
#include "convert.h"

using namespace nanogui;

template<int N, int M, typename Type = Eigen::Matrix<double, N, M>>
class MatrixBox : public Widget {
protected:
    std::function<void(const Type &)> mCallback;
public:
    typedef Eigen::Matrix<double, N, M> MatNM;
    nanogui::FloatBox<double>* inputs[N][M];

    MatrixBox(Widget *parent) : Widget(parent){
        nanogui::GridLayout *gridlayout = new nanogui::GridLayout();
        gridlayout->setResolution(M);
        setLayout(gridlayout);
        for(int j=0;j<N;j++){
            for(int i=0;i<M;i++){
                inputs[j][i] = new FloatBox<double>(this,0);
                inputs[j][i]->setEditable(true);
            }
        }
    }

    /// Set the change callback
    std::function<void(const Type &)> callback() const { return mCallback; }
    void setCallback(const std::function<void(const Type &)> &callback) {

        for(int i=0;i<M;i++){
            for(int j=0;j<N;j++){
                inputs[j][i]->setCallback([&,j,i](const double val){
                    mMatrix(j,i) = val;
                    // fprintf(stderr, "set mat[%d,%d] = %f\n", j, i, val);
                    mCallback(matrix());
                });
            }
        }
        mCallback = callback;
    }

    /// Get the current matrix.
    Type matrix() const {
        return convert<Eigen::Matrix<double, N, M>, Type>(mMatrix);
    }
    MatNM mMatrix;
    /// Set the current color
    void setMatrix(const Type mat){
        mMatrix = convert<Type, Eigen::Matrix<double, N, M>>(mat);
        for(int i=0;i<N;i++){
            for(int j=0;j<M;j++){
                inputs[i][j]->setValue(mMatrix(i,j));
            }
        }
        // fprintf(stderr,"ManSetting Matrix: %f\n",mMatrix(0,0));
    }
};
