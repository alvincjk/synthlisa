#include "lisasim-lisa.h"
#include "lisasim-noise.h"

// --- generic LISA class --------------------------------------------------------------

// This is the generic version of "putn", which calls armlength
// to return the correct retarded photon propagation vector

void LISA::putn(Vector &n,int arms,double t) {
    int arm = abs(arms);

    int crafta = (arm + 1 < 4) ? (arm + 1) : (arm - 2);
    int craftb = (arm + 2 < 4) ? (arm + 2) : (arm - 1);

    if(arms < 0) {
        int swap = crafta;
        crafta = craftb;
        craftb = swap;
    }

    // this is consistent with pa(t) = pb(t-L(t;b->a)) + L(t;b->a) n
    // for instance, p_1 = p_2 + n_3

    Vector pa, pb;

    putp(pa,crafta,t);
    putp(pb,craftb,t-armlength(arms,t));

    for(int i=0;i<3;i++)
        n[i] = pa[i] - pb[i];
    
    // normalize to a unit vector

    double norm = sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);    
                
    for(int i=0;i<3;i++)
        n[i] /= norm;
}

// This is the generic version of "armlength", which takes differences between positions
// looking for a null photon propagation vector

double LISA::armlength(int arms, double t) {
    int arm = abs(arms);

    int crafta = (arm + 1 < 4) ? (arm + 1) : (arm - 2);
    int craftb = (arm + 2 < 4) ? (arm + 2) : (arm - 1);

    if(arms < 0) {
        int swap = crafta;
        crafta = craftb;
        craftb = swap;
    }

    // this is consistent with pa(t) = pb(t-L(t;b->a)) + L(t;b->a) n
    // for instance, p_1 = p_2 + n_3

    Vector pa, pb, n;

    putp(pa,crafta,t);

    // implement a simple bisection search for the correct armlength
    // use a 10% initial bracket

    double newguess = guessL[arm];
    double hi = 1.10 * newguess, lo = 0.90 * newguess;

    double guess;
    
    const double tol = 1e-6;

    do {
        guess = newguess;
    
        putp(pb,craftb,t-guess);

        for(int i=0;i<3;i++)
            n[i] = pa[i] - pb[i];

        // compute the invariant relativistic distance between the events
        // of emission and reception

        double norm = sqrt(-guess*guess + n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);

        if(norm >= 0) {
            // distance is spacelike; increase delay

            lo = guess;
        } else {
            // distance is timelike; reduce delay
        
            hi = guess;
        }
        
        newguess = 0.5 * (hi + lo);
        
    } while( fabs(newguess - guess) > tol );

    return newguess;
}

// --- OriginalLISA LISA class ---------------------------------------------------------

OriginalLISA::OriginalLISA(double L1,double L2,double L3) {
    // take armlengths in seconds (do not convert to years)

    L[1] = L1; L[2] = L2; L[3] = L3;
        
    // construct the p vectors with the required lengths;
                
    double dL = sqrt(2.0 * (L[2]*L[2] + L[3]*L[3]) - L[1]*L[1]);
    double dth = -acos((L[2]*L[2] + L[3]*L[3] - L[1]*L[1])/(2.0 * L[2] * L[3]));
    double th3 = acos((3.0*L[2]*L[2] + L[3]*L[3] - L[1]*L[1])/(2.0 * L[2] * dL));
    
    Vector tp1, tp2, tp3;
    
    tp1[0] = 0.0;                   tp1[1] = 0.0;                 tp1[2] = 0.0;
    tp2[0] = L[3] * cos(dth + th3); tp2[1] = L[3] * sin(dth+th3); tp2[2] = 0.0;
    tp3[0] = L[2] * cos(th3);       tp3[1] = L[2] * sin(th3);     tp3[2] = 0.0;

    // align them so that p1 + p2 + p3 = 0
    // need to do only the x axis, the others are guaranteed
    
    double xoffset = (tp1[0] + tp2[0] + tp3[0]) / 3.0;

    // now assign them to initp; reflect axes and exchange 2<>3 so that for equal arms we get the
    // initp values used in CircularRotating
    
    initp[1][0] = -(tp1[0] - xoffset); initp[1][1] = -tp1[1]; initp[1][2] = -tp1[2];
    initp[2][0] = -(tp3[0] - xoffset); initp[2][1] = -tp3[1]; initp[2][2] = -tp3[2];
    initp[3][0] = -(tp2[0] - xoffset); initp[3][1] = -tp2[1]; initp[3][2] = -tp2[2];

    // now compute the corresponding n's as differences of p's, and normalize
    
    for(int i=0;i<3;i++) {
        initn[1][i] = initp[2][i] - initp[3][i];
        initn[2][i] = initp[3][i] - initp[1][i];
        initn[3][i] = initp[1][i] - initp[2][i];                
    }
        
    for(int j=1;j<4;j++) {
        double norm = sqrt(initn[j][0]*initn[j][0] + initn[j][1]*initn[j][1] + initn[j][2]*initn[j][2]);
        
        for(int i=0;i<3;i++)
            initn[j][i] /= norm;
    }
}

// OriginalLISA does not move; hence no distinction between positive and negative arm

void OriginalLISA::putn(Vector &n,int arm,double t) {
    for(int i=0;i<3;i++)
        n[i] = initn[abs(arm)][i];
}

void OriginalLISA::putp(Vector &p,int craft,double t) {
    for(int i=0;i<3;i++)
        p[i] = initp[craft][i];
}

double OriginalLISA::armlength(int arm, double t) {
    return( L[abs(arm)] );
}

// --- ModifiedLISA LISA class ---------------------------------------------------------

ModifiedLISA::ModifiedLISA(double arm1,double arm2,double arm3) : OriginalLISA(arm1,arm2,arm3) {
    // compute everything in seconds

    for(int i=1;i<4;i++) {
        int crafta = (i + 1 < 4) ? i + 1 : i - 2;
        int craftb = (i + 2 < 4) ? i + 2 : i - 1;

        double La = sqrt(initp[crafta].dotproduct(initp[crafta]));
        double Lb = sqrt(initp[craftb].dotproduct(initp[craftb]));

        // see Mathematica file armlength.nb for this expression

        sagnac[i] = La * Lb * sqrt(1.0 - (La*La + Lb*Lb - L[i]*L[i])*(La*La + Lb*Lb - L[i]*L[i]) / (4.0 * La*La * Lb*Lb)) * Omega;

        Lc[i]  = L[i] + sagnac[i];
        Lac[i] = L[i] - sagnac[i];
    }
}

// implement simple LISA rotation in xy plane with period of a year
// take input time in seconds

void ModifiedLISA::putp(Vector &p,int craft,double t) {
    p[0] = cos(Omega*t) * initp[craft][0] - sin(Omega*t) * initp[craft][1];
    p[1] = sin(Omega*t) * initp[craft][0] + cos(Omega*t) * initp[craft][1];
    p[2] = initp[craft][2];
}

double ModifiedLISA::armlength(int arm, double t) {
    if(arm > 0)
        return(Lc[arm]);
    else
        return(Lac[-arm]);
}

// --- CircularRotating LISA class -----------------------------------------------------

// full constructor; initialize positions and arm vectors
// measure everything in seconds
// if the last argument is negative, switch 2 and 3; needed for coherence with the Montana simulator

CircularRotating::CircularRotating(double myL, double e0, double x0, double sw) {
    // set custom armlength (s)

    L = myL;

    initialize(e0,x0,sw);
}

CircularRotating::CircularRotating(double e0, double x0, double sw) {
    // set standard armlength (s)

    L = Lstd;

    initialize(e0,x0,sw);
}

void CircularRotating::initialize(double e0, double x0, double sw) {
    // set distance from the Sun (s)

    R = Rgc;

    // set distance from guiding center (s)

    scriptl = L / sqrt(3.0);

    // define guessL, in case we wish to call the base-LISA armlength()
    // for comparison with our fitted expressions

    guessL[1] = L; guessL[2] = L; guessL[3] = L;

    // set initial phases

    eta0 = e0;
    xi0 = x0;

    // construct the arms

    initn[1][0] =  0.0; initn[1][1] = -1.0; initn[1][2] = 0.0;
    initn[2][0] = -cos(M_PI/6.0); initn[2][1] = sin(M_PI/6.0); initn[2][2] = 0.0;
    initn[3][0] =  cos(M_PI/6.0); initn[3][1] = sin(M_PI/6.0); initn[3][2] = 0.0;
    
    initp[1][0] = scriptl; initp[1][1] = 0.0; initp[1][2] = 0.0;
    initp[2][0] = scriptl * cos(2.*M_PI/3.0); initp[2][1] = -scriptl * sin(2.*M_PI/3.0); initp[2][2] = 0.0;
    initp[3][0] = scriptl * cos(4.*M_PI/3.0); initp[3][1] = -scriptl * sin(4.*M_PI/3.0); initp[3][2] = 0.0;

    // switch the arms 2 and 3 if required; needed for comparison with Montana simulator

    if (sw < 0.0) {
        double tmp;
        
        for(int i=0;i<3;i++) {
            tmp = initp[2][i];
            initp[2][i] = initp[3][i];
            initp[3][i] = tmp;

            // the arm switch is untested
        
            initn[1][i] = -initn[1][i];
            tmp = initn[2][i];
            initn[2][i] = -initn[3][i];
            initn[3][i] = -tmp;
        }
    }
    
    // set amplitude and phase of delay modulations
    
    delmodamp = delmodconst * R * (scriptl * scriptl) * Omega;
    // delmodamp = 0.0;
    
    delmodph[1] = xi0;
    delmodph[2] = xi0 + 4.*M_PI/3.0;
    delmodph[3] = xi0 + 2.*M_PI/3.0;

    if (sw < 0.0) {
        double tmp;
        
        tmp = delmodph[2];
        delmodph[2] = delmodph[3];
        delmodph[3] = tmp;
        
        delmodamp = -delmodamp;
    }

    settime(0);
}

// Set the correct components of the rotation matrix for the current time
// set also the position of the guiding center

void CircularRotating::settime(double t) {
    const double zeta = -M_PI/6.0;

    // {eta -> Omega time + eta0, xi -> -Omega time + xi0}
    // {psi -> xi, asc -> eta, dec -> zeta}
    // time is measured in seconds
    
    rotation.seteuler(zeta,Omega*t+eta0,-Omega*t+xi0);
    
    // R{Cos[Omega t + eta0], Sin[Omega t + eta0], 0}; leave center[2] = 0.0
    
    center[0] = R * cos(Omega*t+eta0);
    center[1] = R * sin(Omega*t+eta0);
}

// Return the position of "craft" at time t in vector p
// does so by multiplying the initial position of the craft (with respect to the guiding center)
// by a rotation matrix computed at the current time (if it is not already cached)

void CircularRotating::putp(Vector &p,int craft,double t) {
    if (t != rotationtime) settime(t);

    p.setproduct(rotation, initp[craft]);
    
    // forget the z axis for the LISA center
    
    p[0] += center[0];
    p[1] += center[1];
}

// fit to armlength modulation from armlength.nb

double CircularRotating::armlength(int arm, double t) {
    if(arm > 0) {
        return L + delmodamp * sin(Omega*t - delmodph[arm]);
    } else {
        return L - delmodamp * sin(Omega*t - delmodph[-arm]);
    }
}

// call the generic version

double CircularRotating::genarmlength(int arm, double t) {
    return LISA::armlength(arm,t);
}

// call the old putn, but change the sign of n if arm is negative

void CircularRotating::oldputn(Vector &n,int arm,double t) {
    if (t != rotationtime) settime(t);

    n.setproduct(rotation, initn[abs(arm)]);

    if (arm < 0) {
        n[0] = -n[0];
        n[1] = -n[1];
        n[2] = -n[2];
    }
}

// --- MontanaEccentric LISA class -----------------------------------------------------

// full constructor
// define the LISA Simulator kappa and lambda constants

MontanaEccentric::MontanaEccentric(double k0, double l0) {
    L = Lstd;

    // since we do not define armlength, we must initialize guessL
    // to the initial guess for the length of arms

    guessL[1] = L; guessL[2] = L; guessL[3] = L;

    kappa = k0;
    lambda = l0;

    // Initialize the position cache

    settime(1,0.0); settime(2,0.0); settime(3,0.0);
}

// positions of spacecraft according to the LISA simulator

void MontanaEccentric::settime(int craft, double t) {
    const double sqecc = ecc*ecc;
    const double sqrt3 = sqrt(3.0);

    // the indexing of spacecraft is the same with respect to the Simulator
    // (if we do the switch in CircularRotating)
    
    double alpha = Omega*t + kappa;
    double beta = 2.0*M_PI*(craft-1)/3.0 + lambda;

    cachep[craft][0] =   0.5 * Rgc * ecc * ( cos(2.0*alpha-beta) - 3.0*cos(beta) )
                       + 0.125 * Rgc * sqecc * ( 3.0*cos(3.0*alpha-2.0*beta) - 5.0*( 2.0*cos(alpha)+cos(alpha-2.0*beta) ) )
                       + Rgc * cos(alpha);
                          
    cachep[craft][1] =   0.5 * Rgc * ecc * ( sin(2.0*alpha-beta) - 3.0*sin(beta) )
                       + 0.125 * Rgc * sqecc * ( 3.0*sin(3.0*alpha-2.0*beta) - 5.0*( 2.0*sin(alpha)-sin(alpha-2.0*beta) ) )
                       + Rgc * sin(alpha);
           
    cachep[craft][2] = - sqrt3 * Rgc * ecc * cos(alpha-beta) 
                       + sqrt3 * Rgc * sqecc * ( cos(alpha-beta)*cos(alpha-beta) + 2.0*sin(alpha-beta)*sin(alpha-beta) );                                               

    cachetime[craft] = t;
}

void MontanaEccentric::putp(Vector &p, int craft, double t) {
    if (t != cachetime[craft]) settime(craft,t);

    for(int i=0;i<3;i++)
        p[i] = cachep[craft][i];
}

// --- NoisyLISA class -----------------------------------------------------

// the error on the arms is defined in seconds

NoisyLISA::NoisyLISA(LISA *clean,double starm,double sdarm) {
    cleanlisa = clean;
    
    // to estimate size of noisebuffer, take the largest armlength at time zero, and add 10%

    double arm1 = cleanlisa->armlength(1,0.0);
    double arm2 = cleanlisa->armlength(2,0.0);
    double arm3 = cleanlisa->armlength(3,0.0);

    double maxarm = arm1 > arm2 ? arm1 : arm2;
    maxarm = maxarm > arm3 ? maxarm : arm3;

    double lighttime = 1.10 * maxarm;

    // create InterpolateNoise objects for the arm determination noises
    // we need triple retardations

    double pbtarm = 3.0 * lighttime;

    // we use plain uncorrelated white noise, for the moment

    for(int link = 1; link <= 3; link++) {
        uperror[link] = new InterpolateNoise(starm, pbtarm, sdarm, 0.0);
        downerror[link] = new InterpolateNoise(starm, pbtarm, sdarm, 0.0);
    }
}

NoisyLISA::~NoisyLISA() {
    for(int link = 1; link <= 3; link++) {
        delete uperror[link];
        delete downerror[link];
    }
}

void NoisyLISA::reset() {
    for(int link = 1; link <= 3; link++) {
        uperror[link]->reset();
        downerror[link]->reset();
    }    
}

double NoisyLISA::armlength(int arm, double t) {
    if(arm > 0)
        return(cleanlisa->armlength(arm,t) + (*uperror[arm])[t]);
    else
        return(cleanlisa->armlength(arm,t) + (*downerror[-arm])[t]);
}
