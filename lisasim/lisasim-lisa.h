#ifndef _LISASIM_LISA_H_
#define _LISASIM_LISA_H_

#include "lisasim-tens.h"
#include "lisasim-noise.h"

class LISA {
    public:

        LISA() {};

        // default destructor: do I need this?

        virtual ~LISA() {};

        virtual void reset() {};

        virtual void putn(Vector &n, int arm, double t) = 0;
        virtual void putp(Vector &p, int craft, double t) = 0;

        virtual double armlength(int arm, double t);

        // these extra methods are needed to load arrays (not Vector objects)
        // with the spacecraft positions and links

        virtual void putn(double n[], int arm, double t) {
            Vector temp;
            
            putn(temp, arm, t);
            
            n[0] = temp[0];
            n[1] = temp[1];
            n[2] = temp[2];
	}
        
        virtual void putp(double p[], int craft, double t) {
            Vector temp;
            
            putp(temp, craft, t);
            
            p[0] = temp[0];
            p[1] = temp[1];
            p[2] = temp[2];
	}
};

class OriginalLISA : public LISA {
    protected:

    double L[4];
    
    Vector initn[4];
    Vector initp[4];
        
    public:
    
    // accept the armlength in seconds

    OriginalLISA(double arm1,double arm2,double arm3);

    virtual void putn(Vector &n, int arm, double t);
    virtual void putp(Vector &p, int craft, double t);

    virtual double armlength(int arm, double t);
};

// for the moment, use the following class only for TDInoise

class ModifiedLISA : public OriginalLISA {
    public:
    
    double sagnac[4];
    double Lc[4], Lac[4];

    // accept the armlength in seconds

    ModifiedLISA(double arm1,double arm2,double arm3);

    void putn(Vector &n, int arm, double t);
    void putp(Vector &p, int craft, double t);

    double armlength(int arm, double t);
};

// The following classes model the geometry of LISA and can be used also for "signal" TDI

class CircularRotating : public LISA {
    public:
    
        double scriptl;
        double R;
        double L;

        double eta0;
        double xi0;

        // Trick: we use 1-3 indexing for LISA positions and vectors, so we need to allocate 4

        Vector initn[4];
        Vector initp[4];
        
        double rotationtime;
        Vector center;
        Tensor rotation;

        CircularRotating(double eta0 = 0.0,double xi0 = 0.0,double sw = 1.0);
        
        void settime(double t);

        void putn(Vector &n,int arm,double t);
        void putp(Vector &p,int craft,double t);
        
        double armlength(int arm, double t);
};

class MontanaEccentric : public LISA {
    public:
    
        // orbital radius of the guiding center (yrs)
        // LISA simulator has Rgc = 1.49597870660e11 m

        static const double Rgc = 0.0000158233;

        // mean arm length of the LISA detector (yrs)
        // LISA simulator has L = 5.0e9 m

        static const double L = 5.288624035993024E-7;

        // eccentricity of the LISA spacecraft orbits
        // LISA simulator has L/(2.0*sqrt(3.0)*Rgc)
        // with L = 5.0e9, equal to 0.00964837

        static const double ecc = 0.00964839;
        
        // initial azimuthal position of the guiding center
    
        double kappa;
        
        // initial orientation of the spacecraft
        
        double lambda;
        
        // caching the positions of spacecraft
        
        Vector cachep[4];
        double cachetime[4];
        
        // public methods (perhaps not all should be)
        
        MontanaEccentric(double kappa0 = 0.0,double lambda0 = 0.0);

        void settime(int craft,double t);
        
        void putn(Vector &n,int arm,double t);

        void putp(Vector &p,int craft,double t);
};

class NoisyLISA : public LISA {
    private:
        LISA *cleanlisa;

        InterpolateNoise *uperror[4], *downerror[4];

    public:
        NoisyLISA(LISA *clean,double starm,double sdarm);
        ~NoisyLISA();
        
        void reset();
    
        double armlength(int arm, double t);

        void putn(Vector &n, int arm, double t) {
            cleanlisa->putn(n,arm,t);
        }
        
        void putp(Vector &p, int craft, double t) {
            cleanlisa->putp(p,craft,t);
        }

        void putn(double n[], int arm, double t) {
            cleanlisa->putn(n,arm,t);
        }
        
        void putp(double p[], int craft, double t) {
            cleanlisa->putp(p,craft,t);
	}
};

#endif /* _LISASIM_LISA_H_ */
