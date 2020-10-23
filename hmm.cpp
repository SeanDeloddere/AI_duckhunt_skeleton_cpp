#include "hmm.hpp"
#include <math.h>
#include <stdlib.h>

using namespace std;

void BirdHmm::HMM::initializeMatrices() {

	P[0][0] = 0.0005;
	P[0][1] = 0.9995;

	A[0][0] = 0.9995;
	A[0][1] = 0.0005;
	A[1][0] = 0.0005;
	A[1][1] = 0.9995;

	B[0][0] = 0.1;
	B[0][1] = 0.1;
	B[0][2] = 0.1;
	B[0][3] = 0.1;
	B[0][4] = 0.2;
	B[0][5] = 0.1;
	B[0][6] = 0.1;
	B[0][7] = 0.1;
	B[0][8] = 0.1;
	B[1][0] = 0.1;
	B[1][1] = 0.1;
	B[1][2] = 0.1;
	B[1][3] = 0.1;
	B[1][4] = 0.2;
	B[1][5] = 0.1;
	B[1][6] = 0.1;
	B[1][7] = 0.1;
	B[1][8] = 0.1;

/* 	for (int i = 0; i < N; i++) {
		for (int j = 0; j < M; j++) {
			B[i][j] = 1 / (double)M - 0.001;
			if (j == M-1) {
				B[i][j] = 1 / (double)M + (M-1)*0.001;
			}
		}
	} */
}

void BirdHmm::HMM::alpha_beta_pass() {
	c[0] = 0.0;

	for (int i = 0; i < N; i++) {
		int O_t = O[0];
		alpha[0][i] = B[i][O_t] * P[0][i];
		c[0] += alpha[0][i];
	}

	c[0] = (c[0] == 0) ? 0 : 1 / c[0];

	for (int i = 0; i < N; i++) {
		alpha[0][i] *= c[0];
	}


	for (int k = 1; k < T; k++) {
		c[k] = 0;

		for (int i = 0; i < N; i++) {
			float s = 0.0;

			for (int j = 0; j < N; j++) {
				s += A[j][i] * alpha[k - 1][j];
			}

			int O_t = O[k];
			alpha[k][i] = B[i][O_t] * s;
			c[k] += alpha[k][i];
		}

		c[k] = (c[k] == 0) ? 0 : 1 / c[k];

		for (int i = 0; i < N; i++) {
			alpha[k][i] *= c[k];
		}
	}

	for (int i = 0; i < N; i++) {
		beta[T - 1][i] = c[T - 1];
	}

	for (int k = T - 2; k >= 0; k--) {
		for (int i = 0; i < N; i++) {
			beta[k][i] = 0;

			for (int j = 0; j < N; j++) {
				beta[k][i] += A[i][j] * B[j][O[k + 1]] * beta[k + 1][j];
			}

			beta[k][i] = c[k] * beta[k][i];
		}
	}
}

void BirdHmm::HMM::gamma_pass() {
	double den;

	for (int k = 0; k < T - 1; k++) {
		den = 0;
		for (int i = 0; i < N; i++) {
			for (int j = 0; j < N; j++) {
				den += alpha[k][i] * A[i][j] * B[j][O[k + 1]] * beta[k + 1][j];
			}
		}
		for (int i = 0; i < N; i++) {
			gamma[k][i] = 0;
			for (int j = 0; j < N; j++) {
				digamma[k][i][j] = (alpha[k][i] * A[i][j] * B[j][O[k + 1]] * beta[k + 1][j]) / den;
				gamma[k][i] += digamma[k][i][j];
			}
		}
	}

	den = 0;
	for (int i = 0; i < N; i++) {
		den += alpha[T - 1][i];
	}
	for (int i = 0; i < N; i++) {
		gamma[T - 1][i] = alpha[T - 1][i] / den;
	}
}

void BirdHmm::HMM::lambda_pass() {
	double num;
	double den;

	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			num = 0;
			den = 0;
			for (int k = 0; k < T - 1; k++) {
				num += digamma[k][i][j];
				den += gamma[k][i];
			}
			A[i][j] = num/den;
		}
	}

	for (int i = 0; i < N; i++) {
		for (int j = 0; j < M; j++) {
			num = 0;
			den = 0;
			for (int k = 0; k < T; k++) {
				if (j == O[k]) {
					num += gamma[k][i];
				}
				den += gamma[k][i];
			}
			B[i][j] = num/den;
		}
	}

	for (int i = 0; i < N; i++) {
		P[0][i] = gamma[0][i];
	}
}

double BirdHmm::HMM::alpha_s(int* directionObservations, int length) {
	for (int i = 0; i < N; i++) {
		int k = directionObservations[0];
		alpha[0][i] = B[i][k] * P[0][i];
	}

	for (int t = 1; t < length; t++) {
		for (int i = 0; i < N; i++) {
			alpha[t][i] = 0.0;

			for (int j = 0; j < N; j++) {
				alpha[t][i] += A[j][i] * alpha[t - 1][j];
			}

			int k = directionObservations[t];
			alpha[t][i] = B[i][k] * alpha[t][i];
		}

	}
	
	//P(O|lamba) = SUM(i: 0->N-1) alpha[T-1][i]
	double s = 0.0;

	for (int i = 0; i < N; i++) {
		s += alpha[length - 1][i] / c[length - 1];
	}

	return s;
}

int BirdHmm::HMM::predictMove() {
	int move = -1;
	double maxProb = 0.0;
	double s;

	for (int i = 0; i < 9; i++) {
		s = 0.0;
		for (int j = 0; j < N; j++) {
			s += (alpha[T - 1][j]) * B[j][i];
		}
		if (s > maxProb) {
			maxProb = s;
			move = i;
		}
	}
	if (maxProb < 0.6)
		return -1;
	return move;
}

void BirdHmm::HMM::estimateModel(int max_it) {
	int it = 0;
	double old_LogProb = DBL_MIN;

	converged = false;
	while (it < max_it) {
		it++;

		alpha_beta_pass();
		gamma_pass();
		lambda_pass();

		double logProb = 0;
		for (int i = 0; i < T; i++) {
			logProb += log(c[i]);
		}

		logProb = -logProb;
		if (logProb > old_LogProb) {
			old_LogProb = logProb;
		}
		else {
			converged = true;
			break;
		}
	}
}

//s========================================================getters & setters========================================================
void BirdHmm::HMM::setT(int newTime) {
	T = newTime;
}

void BirdHmm::HMM::setO(int direction, int i) {
	O[i] = direction;
}

bool BirdHmm::HMM::getConverged() {
	bool c = converged;
	return c;
}