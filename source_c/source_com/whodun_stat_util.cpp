#include "whodun_stat_util.h"

#include <math.h>
#include <float.h>
#include <stdint.h>

using namespace whodun;

ProbabilitySummation::ProbabilitySummation(){
	runningSum = 0.0;
	maxLogProb = 0.0/0.0;
	nextQInd = 0;
}

void ProbabilitySummation::updateFromQueue(){
	//get the maximum currently in the queue
	double queueMax = queuedVals[0];
	for(int i = 1; i<nextQInd; i++){
		if(queuedVals[i] > queueMax){
			queueMax = queuedVals[i];
		}
	}
	//if it is larger than the current winner, shift
	if(isnan(maxLogProb) || (queueMax > maxLogProb)){
		if(!isnan(maxLogProb)){
			//rescale the current sum
			double rescaleFac = pow(10.0, maxLogProb - queueMax);
			runningSum = runningSum * rescaleFac;
		}
		maxLogProb = queueMax;
	}
	//add to the running sum
	double curSum = 0.0;
	for(int i = 0; i<nextQInd; i++){
		curSum = curSum + pow(10.0, queuedVals[i] - maxLogProb);
	}
	runningSum += curSum;
	//reset
	nextQInd = 0;
}

void ProbabilitySummation::addNextLogProb(double nextLogProb){
	queuedVals[nextQInd] = nextLogProb;
	nextQInd++;
	if(nextQInd >= WHODUN_PROB_SUMMATION_QUEUE){
		updateFromQueue();
	}
}

double ProbabilitySummation::getFinalLogSum(){
	if(nextQInd){
		updateFromQueue();
	}
	return maxLogProb + log10(runningSum);
}

#define EULERMASCHERONI 0.5772156649015328606065120900824024310421

double whodun::logGammaT(double forVal, double useEpsilon){
	#ifdef C99_FALLBACK
		if(forVal <= 0.0){
			return 1.0 / 0.0;
		}
		double fullTot = - EULERMASCHERONI * forVal;
		fullTot -= log(forVal);
		uintptr_t curK = 1;
		while(true){
			double curAdd = forVal/curK - log(1.0 + forVal/curK);
			if(fabs(fullTot) > useEpsilon){
				if(fabs(curAdd/fullTot) < useEpsilon){
					break;
				}
			}
			else if(fabs(curAdd) < useEpsilon){
				break;
			}
			fullTot += curAdd;
			curK++;
		}
		return fullTot;
	#else
		return lgamma(forVal);
	#endif
}

double whodun::logGamma(double forVal){
	#ifdef C99_FALLBACK
		return logGammaT(forVal, DBL_EPSILON);
	#else
		return lgamma(forVal);
	#endif
}

double whodun::expMinusOneT(double forVal, double useEpsilon){
	#ifdef C99_FALLBACK
		if((forVal < -0.69315) || (forVal > 0.40547)){
			return exp(forVal) - 1.0;
		}
		double toRet = 0;
		long curDen = 1;
		double toAdd = forVal;
		while(true){
			if(fabs(toRet) > useEpsilon){
				if(fabs(toAdd/toRet) < useEpsilon){
					break;
				}
			}
			else if(fabs(toAdd) < useEpsilon){
				break;
			}
			toRet += toAdd;
			curDen++;
			toAdd = toAdd * (forVal / curDen);
		}
		return toRet;
	#else
		return expm1(forVal);
	#endif
}

double whodun::expMinusOne(double forVal){
	#ifdef C99_FALLBACK
		return expMinusOneT(forVal, DBL_EPSILON);
	#else
		return expm1(forVal);
	#endif
}

double whodun::logOnePlusT(double forVal, double useEpsilon){
	#ifdef C99_FALLBACK
		//need to do this better
		return log(1 + forVal);
		//TODO
	#else
		return log1p(forVal);
	#endif
}

double whodun::logOnePlus(double forVal){
	#ifdef C99_FALLBACK
		return logOnePlusT(forVal, DBL_EPSILON);
	#else
		return log1p(forVal);
	#endif
}

double whodun::logOneMinusExpT(double forVal, double useEpsilon){
	if(forVal < 0.693){
		return log(-expMinusOneT(-forVal, useEpsilon));
	}
	else{
		return logOnePlusT(-exp(-forVal), useEpsilon);
	}
}

double whodun::logOneMinusExp(double forVal){
	//ln(2)
	if(forVal < 0.693){
		return log(-expMinusOne(-forVal));
	}
	else{
		return logOnePlus(-exp(-forVal));
	}
}

