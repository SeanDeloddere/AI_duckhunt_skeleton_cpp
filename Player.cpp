#include "Player.hpp"
#include "hmm.hpp"
#include <cstdlib>
#include <iostream>
#include <queue>

using namespace std;



int directionObservations[20][100];
vector<BirdHmm::HMM> hmmVector;

namespace ducks
{
	Player::Player()
	{
	}

	class birdType {
	public:
		ducks::Bird bird;
		ducks::ESpecies species;
	};

	//std::queue<birdType> birds;

	volatile int curBirdNumber = 0;
	volatile int roundNumber = 0;

	int attempts = 0;
	int hits = 0;
	int runIterations = 1000;
	
	Action Player::shoot(const GameState& pState, const Deadline& pDue)
	{
		/*
		 * Here you should write your clever algorithms to get the best action.
		 * This skeleton never shoots.
		 */

		 /*
		 There will be up to 10 rounds in each environment. Each round contains 1-20 birds and goes on
		 for 100 time steps. The round will end when all birds are shot or when time runs out.
		 Every curBirdNumber in the sky makes a move for every discrete time step that the game runs.
		 No moves are reported for birds that have been shot.
		 */

		int numBirds = pState.getNumBirds();
		
		if (!(roundNumber == pState.getRound())) {
			curBirdNumber = curBirdNumber % numBirds;
			roundNumber = pState.getRound();
		}

		if (pDue.remainingMs() < 30 || pState.getRound() == 0) {
			cerr << "Dont shoot: 1st round" << endl;
			return cDontShoot;
		}

		

		while (pState.getBird(curBirdNumber).isDead()) {
			curBirdNumber++;
			curBirdNumber = curBirdNumber % numBirds;
		}

		double shotTiming = (pState.getNumPlayers() > 1) ? 3.0 : 1.5;
		int beginShooting = 99 - (int)(shotTiming * numBirds);

		if (pState.getBird(curBirdNumber).getSeqLength() < beginShooting) {
			curBirdNumber++;
			curBirdNumber = curBirdNumber % numBirds;
			cerr << "Dont shoot: gather more information" << endl;
			return cDontShoot;
		}


		double probability = 0;
		int bestHmm = 0;
		//cerr << "birdnumber: " << curBirdNumber << endl;
		Bird curBird = pState.getBird(curBirdNumber);
		int dirObs[curBird.getSeqLength()];
		
		for (int j = 0; j < curBird.getSeqLength(); j++) {
			if (curBird.getObservation(j) > 8 || curBird.getObservation(j) < -1) {
				dirObs[j] = 0;
				cerr << "Dont shoot: invalid observation:" << endl;
				cerr << "curBird: " << curBirdNumber << ", observation: " << curBird.getObservation(j) << endl;
				return cDontShoot;
			}
			else {
				dirObs[j] = (int)curBird.getObservation(j);
			}
		}

		//get the best model
		for (int k = 0; k < hmmVector.size(); k++) {
			double auxProbability = hmmVector[k].alpha_s(dirObs, (int)curBird.getSeqLength());
			if (auxProbability > probability) {
				probability = auxProbability;
				bestHmm = k;
			}
		}

		ESpecies species = hmmVector[bestHmm].birdSpecies;

		
		BirdHmm::HMM auxHmm;
		auxHmm.setT(pState.getBird(curBirdNumber).getSeqLength());
		for (int j = 0; j < pState.getBird(curBirdNumber).getSeqLength(); j++) {
			auxHmm.setO((int)pState.getBird(curBirdNumber).getObservation(j),j);
		}
		auxHmm.estimateModel(runIterations);
		
		int predictedMove = auxHmm.predictMove();

		//cerr << "numBirds: " << numBirds << endl;

		int aimedBird = curBirdNumber;
		curBirdNumber++;;
		curBirdNumber = curBirdNumber % numBirds;
		if (species != SPECIES_BLACK_STORK && auxHmm.getConverged() && predictedMove != -1 && aimedBird < numBirds) {
			attempts++;
			cerr << "attempting shot on bird: " << aimedBird << endl;
			return Action(aimedBird, (EMovement)predictedMove);
		}
		else {
			return cDontShoot;
		}
	}

	std::vector<ESpecies> Player::guess(const GameState& pState, const Deadline& pDue)
	{
		/*
		 * Here you should write your clever algorithms to guess the species of each bird.
		 * This skeleton makes no guesses, better safe than sorry!
		 */
		int numBirds = pState.getNumBirds();

		std::vector<ESpecies> speciesGuesses(numBirds, SPECIES_PIGEON);

		double maxProb = 0.0;
		int maxJ = 0;
		double temp = 0.0;
		for (int i = 0; i < numBirds && pDue.remainingMs() > 400; i++) {
			maxProb = 0.0;
			for (int j = 0; j < pState.getBird(i).getSeqLength(); j++) {
				directionObservations[i][j] = (int)pState.getBird(i).getObservation(j);
			}
				
			
			for (int j = 0; j < hmmVector.size(); j++) {
				temp = hmmVector[j].alpha_s(directionObservations[i], pState.getBird(i).getSeqLength());
				if (temp > maxProb) {
					maxProb = temp;
					maxJ = j;
				}
			}

			if (!hmmVector.empty()) {
				speciesGuesses[i] = hmmVector[maxJ].birdSpecies;
			}
		}

		cerr << "============================================================" << endl;
		cerr << "GUESS:" << endl;
		for (int i = 0; i < speciesGuesses.size(); i++) {
			cerr << speciesGuesses[i] << ", ";
		}
		cerr << endl;

		return speciesGuesses;
	}

	void Player::hit(const GameState& pState, int pBird, const Deadline& pDue)
	{
		/*
		 * If you hit the bird you are trying to shoot, you will be notified through this function.
		 */
		hits++;
		std::cerr << "HIT BIRD!!!" << std::endl;
		std::cerr << "attempts: " << attempts <<", hits: "<< hits << std::endl;
	}

	void Player::reveal(const GameState& pState, const std::vector<ESpecies>& pSpecies, const Deadline& pDue)
	{
		/*
		 * If you made any guesses, you will find out the true species of those birds in this function.
		 */

		cerr << "REVEAL:" << endl;
		for (int i = 0; i < pSpecies.size(); i++) {
			cerr << pSpecies[i] << ", ";
		}
		cerr << endl;
		cerr << "============================================================" << endl;

		for (int i = 0; i < pState.getNumBirds() && pDue.remainingMs() > 400; i++) {
			if (pSpecies[i] != SPECIES_UNKNOWN) {
				BirdHmm::HMM auxHmm;
				auxHmm.setT(pState.getBird(i).getSeqLength());
				for (int j = 0; j < pState.getBird(i).getSeqLength(); j++) {
					auxHmm.setO((int)pState.getBird(i).getObservation(j),j);
				}
				auxHmm.estimateModel(runIterations);
				auxHmm.birdSpecies = pSpecies[i];
				if (auxHmm.getConverged()) {
					hmmVector.push_back(auxHmm);
				}
				else {
					cerr << "didn't converge" << endl;
				}
			}
		}
	}


} /*namespace ducks*/
