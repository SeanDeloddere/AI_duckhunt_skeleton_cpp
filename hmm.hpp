#ifndef _hmm_h
#define _hmm_h
#include <iostream>
#include <vector>
#include "Player.hpp"

#include <float.h>

#include <time.h>
#include <random>

using namespace std;
namespace BirdHmm {
	class HMM {
	private:
		int N = 2;
		int M = 9;
		int T;
		int O[100];
		double A[2][2];
		double B[2][9];
		double P[2][2];
		double alpha[100][2];
		double beta[100][2];
		double c[100];
		double gamma[100][2];
		double digamma[100][2][2];
		bool converged = false;
		void alpha_beta_pass();
		void gamma_pass();
		void lambda_pass();
		void initializeMatrices();
		
		
		

	public:
		ducks::ESpecies birdSpecies;
		HMM() {
			initializeMatrices();
		}
		
		double alpha_s(int* newSeq, int len);
		void estimateModel(int maxit);
		int predictMove();
		void setT(int newTime);
		void setO(int direction, int i);
		bool getConverged();
	};
}
#endif