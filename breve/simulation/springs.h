enum slSpringModes {
	SPRING_MODE_NORMAL = 0,
	SPRING_MODE_EXPAND_ONLY,
	SPRING_MODE_CONTRACT_ONLY
};

#ifdef __cplusplus
class slSpring: public slObjectConnection {
	public:
		slLink *_src;
		slLink *_dst;

		void slSpring::draw();

		double length;
		double strength;
		double damping;

		unsigned char mode;

		slVector point1;
		slVector point2;
};
#endif

void slWorldApplySpringForces(slWorld *w);
void slSpringApplyForce(slSpring *spring);

#ifdef __cplusplus
extern "C" {
#endif
slSpring *slSpringNew(slLink *l1, slLink *l2, slVector *v1, slVector *v2, double length, double strength, double damping);

double slSpringGetCurrentLength(slSpring *s);
double slSpringGetLength(slSpring *s);

void slSpringSetLength(slSpring *s, double length);
void slSpringSetStrength(slSpring *s, double strength);
void slSpringSetDamping(slSpring *s, double damping);
void slSpringSetMode(slSpring *s, int mode);

void slWorldRemoveSpring(slWorld *w, slSpring *s);
void slWorldAddSpring(slWorld *w, slSpring *s);
void slWorldDrawSprings(slWorld *w);

void slSpringFree(slSpring *spring);
#ifdef __cplusplus
}
#endif
