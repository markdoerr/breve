/*****************************************************************************
 *                                                                           *
 * The breve Simulation Environment                                          *
 * Copyright (C) 2000, 2001, 2002, 2003 Jonathan Klein                       *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program; if not, write to the Free Software               *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
 *****************************************************************************/

/*!
	The multibody is a logical set of links.  It's a slightly different
	paradigm from the independant body/joint system that ODE uses.  If 
	you move/rotate the multibody, for example, all of the links are
	moved/rotated with it.  
	
	The only real rule is that there is exactly one "root" link chosen
	for each multibody.  
*/

#include "simulation.h"

/*
	\brief Sets the label associated with this multibody.
*/

void slMultibodySetLabel(slMultibody *m, char *label) {
	if(m->label) slFree(m->label);
	m->label = slStrdup(label);
}

/*!
	\brief Ignores collisions between adjacent multibody links.
*/

void slMultibodyInitCollisionFlags(slMultibody *m, slPairEntry **pe) {
	unsigned int x;
	slLink *link1, *link2;
	slPairEntry *e;
	std::vector<slLink*>::iterator i1;
	std::vector<slLink*>::iterator i2;

	i1 = m->links.begin();
	i2 = m->links.begin();

	for(i1 = m->links.begin(); i1 != m->links.end(); i1++) {
		link1 = *i1;

		// set all check flags on or off, depending on whether self collision
		// are handled or not.

		for(i2 = m->links.begin(); i2 != m->links.end(); i2++) {
			link2 = *i2;

			if(link1 != link2) {
				e = slVclipPairEntry(pe, link1->clipNumber, link2->clipNumber);

				if(m->handleSelfCollisions) e->flags |= BT_CHECK;
				else if(e->flags & BT_CHECK) e->flags ^= BT_CHECK;
			}
		}

		// then prune out all adjacent pairs.

		for(x=0;x<link1->outJoints->count;x++) {
			link2 = ((slJoint*)link1->outJoints->data[x])->child;

			if(link1 != link2) {
				e = slVclipPairEntry(pe, link1->clipNumber, link2->clipNumber);

				if(e->flags & BT_CHECK) e->flags ^= BT_CHECK;
			}
		}
	}
}

/*!
	\brief Sets the location and/or rotation of a multibody.  

	The location and/or rotation may be NULL.
*/

void slMultibodyPosition(slMultibody *m, slVector *location, double rot[3][3]) {
	slVector offset;

	if(!m->root) return;

	slMultibodyUpdatePositions(m);

	if(location) {
		slVectorSub(location, &m->root->position.location, &offset);
		slMultibodyOffsetPosition(m, &offset);
	}

	if(rot) {
		double invR[3][3], transform[3][3];

		/* figure out the relative rotation required to take us from the 
		 * current rotation to the new one */

		slMatrixTranspose(m->root->position.rotation, invR);
		slMatrixMulMatrix(rot, invR, transform);

		slMultibodyRotate(m, transform);
	}

	slMultibodyUpdatePositions(m);
}

/*!
	\brief Gives a rotation matrix from an axis and an angle.
*/

void slMultibodyRotAngleToMatrix(slVector *axis, double r, double rot[3][3]) {
	slQuat q;
	slQuatSetFromAngle(&q, r, axis);
	slQuatToMatrix(&q, rot);
}

/*!
	\brief Preforms a relative rotation.
*/

void slMultibodyRotate(slMultibody *mb, double rotation[3][3]) {
	slLink *link;
	slVector toLink, newToLink;
	double newRot[3][3];
	dQuaternion Q;
	slQuat q;
	std::vector<slLink*>::iterator i1;

	for(i1 = mb->links.begin(); i1 != mb->links.end(); i1++) {	
		link = *i1;

		// first, since the whole body is making this rotation, 
		// our location will be changed.  we have to compute 
		// the new position based on the rotation angle and our
		// location relative to the root link.

		slVectorSub(&link->position.location, &mb->root->position.location, &toLink);
		slVectorXform(rotation, &toLink, &newToLink);
		slVectorAdd(&newToLink, &mb->root->position.location, &newToLink);
		dBodySetPosition(link->odeBodyID, newToLink.x, newToLink.y, newToLink.z);
	
		slMatrixMulMatrix(rotation, link->position.rotation, newRot);

		slMatrixCopy(newRot, link->position.rotation);

		// update the ODE state

		slMatrixToQuat(newRot, &q);

		Q[0] = q.s;
		Q[1] = q.x;
		Q[2] = q.y;
		Q[3] = q.z;

		if(link->simulate) dBodySetQuaternion(link->odeBodyID, Q);
	}
}

/*!
	\brief Offsets the position of a multibody.
*/

void slMultibodyOffsetPosition(slMultibody *mb, slVector *offset) {
	slLink *link;
	const dReal *oldP;
	dReal newP[3];
	std::vector<slLink*>::iterator i1;

	for(i1 = mb->links.begin(); i1 != mb->links.end(); i1++) {	
		link = *i1;

		slVectorAdd(&link->position.location, offset, &link->position.location);

		if(link->simulate) {
			oldP = dBodyGetPosition(link->odeBodyID);

			newP[0] = oldP[0] + offset->x;
			newP[1] = oldP[1] + offset->y;
			newP[2] = oldP[2] + offset->z;
			
			dBodySetPosition(link->odeBodyID, newP[0], newP[1], newP[2]);
		}
	}
}

/*!
	\brief Gives the position of a joint, relative to its native position.
*/

void slJointGetPosition(slJoint *j, slVector *r) {
	switch(j->type) {
		case JT_REVOLUTE:
			r->x = dJointGetHingeAngle(j->odeJointID);
			r->y = 0;
			r->z = 0;
			break;
		case JT_PRISMATIC:
			r->x = dJointGetSliderPosition(j->odeJointID);
			r->y = 0;
			r->z = 0;
			break;
		case JT_BALL:
		case JT_UNIVERSAL:
			r->x = dJointGetAMotorAngle(j->odeMotorID, dParamVel);
			r->y = dJointGetAMotorAngle(j->odeMotorID, dParamVel2);
			r->z = dJointGetAMotorAngle(j->odeMotorID, dParamVel3);
			break;
		default:
			break;
	}

	return;
}

/*!
	\brief Gets the joint velocity of a joint.

	Returns the 1-DOF value for backwards compatability, but also
	fills in the velocity vector.
*/

void slJointGetVelocity(slJoint *j, slVector *velocity) {
	switch(j->type) {
		case JT_REVOLUTE:
			velocity->x = dJointGetHingeAngleRate(j->odeJointID);
			break;
		case JT_PRISMATIC:
			velocity->x = dJointGetSliderPositionRate(j->odeJointID);
			break;
		case JT_BALL:
			velocity->z = dJointGetAMotorParam(j->odeMotorID, dParamVel3);
		case JT_UNIVERSAL:
			velocity->x = dJointGetAMotorParam(j->odeMotorID, dParamVel);
			velocity->y = dJointGetAMotorParam(j->odeMotorID, dParamVel2);
			break;
	}
}

/*!
	\brief Sets the velocity of a joint.

	The joint may be 1-, 2- or 3DOF, so only the relevant fields 
	of the speed vector are used.
*/

void slJointSetVelocity(slJoint *j, slVector *speed) {
	j->targetSpeed = speed->x;

	// if(j->type == JT_REVOLUTE) dJointSetHingeParam (j->odeJointID, dParamVel, speed->x);
	// else if(j->type == JT_PRISMATIC) dJointSetSliderParam (j->odeJointID, dParamVel, speed->x);

	if(j->type == JT_UNIVERSAL) {
		dJointSetAMotorParam(j->odeMotorID, dParamVel, speed->x);
		dJointSetAMotorParam(j->odeMotorID, dParamVel3, speed->y);
	} else if(j->type == JT_BALL) {
		dJointSetAMotorParam(j->odeMotorID, dParamVel, speed->x);
		dJointSetAMotorParam(j->odeMotorID, dParamVel2, speed->y);
		dJointSetAMotorParam(j->odeMotorID, dParamVel3, speed->z);
	}
}

/*!
	\brief Sets the linear and/or rotational velocities of a link.
*/

void slMultibodySetVelocity(slMultibody *mb, slVector *linear, slVector *rotational) {
	slLink *link;
	std::vector<slLink*>::iterator i;

	for(i=mb->links.begin(); i != mb->links.end(); i++) {
		link = *i;

		slLinkSetVelocity(link, linear, rotational);
	}
}

/*!
	\brief Sets the linear and/or rotational accelerations of a link.
*/

void slMultibodySetAcceleration(slMultibody *mb, slVector *linear, slVector *rotational) {
	slLink *link;
	std::vector<slLink*>::iterator i;

	for(i=mb->links.begin(); i != mb->links.end(); i++) {
		link = *i;

		slLinkSetAcceleration(link, linear, rotational);
	}
}


/*!
	\brief Frees a multibody, but does not free its links.

	The links will continue to exist independent of the multibody
	and must be freed seperately.
*/

void slMultibodyFree(slMultibody *m) {
	slNullOrphanMultibodies(m->root);
	m->links.empty();

	delete m;
}

/*!
	\brief Returns all of the callback data for links and joints in a multibody.
*/

slList *slMultibodyAllCallbackData(slMultibody *mb) {
	slList *list = NULL;
	slLink *link;
	unsigned int n;
	std::vector<slLink*>::iterator i;

	for( i = mb->links.begin(); i != mb->links.end(); i++ ) {
		link = *i;
		
		list = slListPrepend(list, link->callbackData);

		for(n=0;n<link->inJoints->count;n++) {
			slJoint *j = link->inJoints->data[n];

			list = slListPrepend(list, j->callbackData);
		}
	}

	return list;
}

/*!
	\brief Sets minima and maxima for a joint.

	The minima and maxima are relative to the joint's natural state.
	Since the joint may be 1-, 2- or 3-DOF, only the relevant field
	of the vectors are used.
*/

void slJointSetLimits(slJoint *joint, slVector *min, slVector *max) {
	switch(joint->type) {
		case JT_PRISMATIC:	
			dJointSetSliderParam(joint->odeJointID, dParamStopERP, .1);
			dJointSetSliderParam(joint->odeJointID, dParamLoStop, min->x);
			dJointSetSliderParam(joint->odeJointID, dParamHiStop, max->x);
			break;
		case JT_REVOLUTE:	
			dJointSetHingeParam(joint->odeJointID, dParamStopERP, .1);
			dJointSetHingeParam(joint->odeJointID, dParamLoStop, min->x);
			dJointSetHingeParam(joint->odeJointID, dParamHiStop, max->x);
			break;
		case JT_BALL:	
			if(max->y >= M_PI/2.0 - 0.001) max->y = M_PI/2.0 - 0.001;
			if(min->y <= -M_PI/2.0 - 0.001) min->y = -M_PI/2.0 + 0.001;

			dJointSetAMotorParam(joint->odeMotorID, dParamLoStop, min->x);
			dJointSetAMotorParam(joint->odeMotorID, dParamLoStop2, min->y);
			dJointSetAMotorParam(joint->odeMotorID, dParamLoStop3, min->z);
			dJointSetAMotorParam(joint->odeMotorID, dParamStopERP, .1);
			dJointSetAMotorParam(joint->odeMotorID, dParamStopERP2, .1);
			dJointSetAMotorParam(joint->odeMotorID, dParamStopERP3, .1);
			dJointSetAMotorParam(joint->odeMotorID, dParamHiStop, max->x);
			dJointSetAMotorParam(joint->odeMotorID, dParamHiStop2, max->y);
			dJointSetAMotorParam(joint->odeMotorID, dParamHiStop3, max->z);
			break;
		case JT_UNIVERSAL:
			dJointSetAMotorParam(joint->odeMotorID, dParamLoStop, min->x);
			dJointSetAMotorParam(joint->odeMotorID, dParamLoStop3, min->y);
			dJointSetAMotorParam(joint->odeMotorID, dParamStopERP, .1);
			dJointSetAMotorParam(joint->odeMotorID, dParamStopERP3, .1);
			dJointSetAMotorParam(joint->odeMotorID, dParamHiStop, max->x);
			dJointSetAMotorParam(joint->odeMotorID, dParamHiStop3, max->y);
			break;
	}
}

/*!
	\brief Set the maximum torque that a joint can affect.
*/

void slJointSetMaxTorque(slJoint *joint, double max) {
	switch(joint->type) {
		case JT_REVOLUTE:
			dJointSetHingeParam(joint->odeJointID, dParamFMax, max);
			break;
		case JT_PRISMATIC:
			dJointSetSliderParam(joint->odeJointID, dParamFMax, max);
			break;
		case JT_UNIVERSAL:
			dJointSetAMotorParam(joint->odeMotorID, dParamFMax, max);
			dJointSetAMotorParam(joint->odeMotorID, dParamFMax3, max);
			break;
		case JT_BALL:
			dJointSetAMotorParam(joint->odeMotorID, dParamFMax, max);
			dJointSetAMotorParam(joint->odeMotorID, dParamFMax2, max);
			dJointSetAMotorParam(joint->odeMotorID, dParamFMax3, max);
			break;
	}
}

/*!
	\brief Frees an slJoint struct.
*/

void slJointDestroy(slJoint *joint) {
	slFree(joint);
}

/*!
	\brief Breaks an slJoint struct.

	This triggers an automatic recomputation of multibodies.
*/

slLink *slJointBreak(slJoint *joint) {
	slLink *parent = joint->parent, *child = joint->child;
	slMultibody *parentBody = NULL, *childBody, *newMb;

	if(!parent && !child) return NULL;

	childBody = joint->child->multibody;

	if(parent) parentBody = parent->multibody;

	if(parent) slStackRemove(parent->outJoints, joint);
	slStackRemove(child->inJoints, joint);

	dJointAttach(joint->odeJointID, NULL, NULL);
	dJointDestroy(joint->odeJointID);

	if(joint->odeMotorID) {
		dJointAttach(joint->odeMotorID, NULL, NULL);
		dJointDestroy(joint->odeMotorID);
	}

	joint->child = NULL;
	joint->parent = NULL;

	if(parentBody) slMultibodyUpdate(parentBody);
	if(childBody && childBody != parentBody) slMultibodyUpdate(childBody);

	/* figure out if the broken links are still part of those bodies */
	/* ... if not, then try to adopt the links */
	/* ... if not, then NULL the multibody entries */

	if(parent && parentBody && (std::find(parentBody->links.begin(), parentBody->links.end(), parent) != parentBody->links.end())) {
		if((newMb = slLinkFindMultibody(parent))) slMultibodyUpdate(newMb);
		else slNullOrphanMultibodies(parent);
	}

	if(child && childBody && (std::find(childBody->links.begin(), childBody->links.end(), child) != childBody->links.end())) {
		if((newMb = slLinkFindMultibody(child))) slMultibodyUpdate(newMb);
		else slNullOrphanMultibodies(child);
	}

	// the bodies may have changed...

	return parent;
}

/*!
	\brief NULL out the multibody fields of orphaned link subtrees.
*/

void slNullOrphanMultibodies(slLink *orphan) {
	slList *o, *orphans;

	o = orphans = slLinkList(orphan, NULL, 0);
	
	while(orphans) {
		((slLink*)orphans->data)->multibody = NULL;

		orphans = orphans->next;
	}

	slListFree(o);
}

/*!
	\brief Find a multibody that a link is attached to.

	This function is only used when a link must recompute its own 
	multibody, such as after a joint break.
*/

slMultibody *slLinkFindMultibody(slLink *root) {
	slList *start, *rootList = slLinkList(root, NULL, 0);
	slStack *jointStack;
	slJoint *joint;
	unsigned int n;

	start = rootList;

	while(rootList) {
		slLink *link = rootList->data;

		if(link != root && root->multibody != link->multibody) {
			/* okay!  link has another mb for us!  we need to find the joint */
			/* connecting this new multibody to our old one! */

			jointStack = link->inJoints;

			for(n=0;n<jointStack->count;n++) {
				joint = jointStack->data[n];

				if(joint->parent && joint->parent->multibody == root->multibody) {
					joint->isMbJoint = 1;
					slListFree(start);
					return link->multibody;
				}
			}

			jointStack = link->outJoints;

			for(n=0;n<jointStack->count;n++) {
				joint = jointStack->data[n];

				if(joint->child->multibody == root->multibody) {
					joint->isMbJoint = 1;
					slListFree(start);
					return link->multibody;
				}
			}
		}

		rootList = rootList->next;
	}

	slListFree(start);

	return NULL;
}

/*!
	\brief Returns a list of links connected to a root link.
*/

slList *slLinkList(slLink *root, slList *list, int mbOnly) {
	unsigned int n;

	if(!root || slInList(list, root)) return list; 

	list = slListPrepend(list, root);

	for(n=0;n<root->outJoints->count;n++) {
		if(!mbOnly || ((slJoint*)root->outJoints->data[n])->isMbJoint) 
			list = slLinkList(((slJoint*)root->outJoints->data[n])->child, list, mbOnly);
	}

	for(n=0;n<root->inJoints->count;n++) {
		if(!mbOnly || ((slJoint*)root->inJoints->data[n])->isMbJoint) 
			list = slLinkList(((slJoint*)root->inJoints->data[n])->parent, list, mbOnly);
	}

	return list;
}

void slMultibodyComputeLinks(slMultibody *mb, int mbOnly) {

}

/*!
	\brief Counts multibody links and sets the links' mb fields.
*/

int slMultibodyCountLinks(slMultibody *mb) {
	int number = 0;
	std::vector<slLink*>::iterator i;

	for( i = mb->links.begin(); i != mb->links.end(); i++ ) {	
		slLink *link = *i;

		link->multibody = mb;
		number++;
	}

	return number;
}

/*!
	\brief Computes the total mass of a multibody.
*/

double slMultibodyComputeMass(slMultibody *mb) {
	slLink *link;
	double mass = 0.0;
	std::vector<slLink*>::iterator i;

	for(i = mb->links.begin(); i != mb->links.end(); i++ ) {
		link = *i;

		mass += link->shape->mass;
	}

	return mass;
}

/*!
	\brief Sets the normal vector of a prismatic or revolute joint.
*/

int slJointSetNormal(slJoint *joint, slVector *normal) {
	if(joint->type == JT_REVOLUTE) {
		dJointSetHingeAxis(joint->odeJointID, normal->x, normal->y, normal->z);
	} else if(joint->type == JT_PRISMATIC) {
		dJointSetSliderAxis(joint->odeJointID, normal->x, normal->y, normal->z);
	}	

	return 0;
}

/*!
	\brief Modifies the link points of a joint.
*/

int slJointSetLinkPoints(slJoint *joint, slVector *plinkPoint, slVector *clinkPoint, double rotation[3][3]) {
	const double *childR;
	dReal idealR[16];
	dReal savedChildR[16];
	slVector hingePosition, childPosition;
	double ideal[3][3];

	childR = dBodyGetRotation(joint->child->odeBodyID);
	bcopy(childR, savedChildR, sizeof(dReal) * 16);

	if(joint->parent) {
		slMatrixMulMatrix(joint->parent->position.rotation, rotation, ideal);
	} else {
		slMatrixCopy(rotation, ideal);		
	}

	slSlToODEMatrix(ideal, idealR);

	/* compute the hinge position--the plinkPoint in world coordinates */

	if(joint->parent) {
		slVectorXform(joint->parent->position.rotation, plinkPoint, &hingePosition);
		slVectorAdd(&hingePosition, &joint->parent->position.location, &hingePosition);
	} else {
		slVectorCopy(plinkPoint, &hingePosition);
	}

	/* set the ideal positions, so that the anchor command registers the native position */

	slVectorXform(ideal, clinkPoint, &childPosition);
	slVectorSub(&hingePosition, &childPosition, &childPosition);

	dJointAttach(joint->odeJointID, NULL, NULL);

	dBodySetRotation(joint->child->odeBodyID, idealR);
	dBodySetPosition(joint->child->odeBodyID, childPosition.x, childPosition.y, childPosition.z);

	if(joint->parent) dJointAttach(joint->odeJointID, joint->parent->odeBodyID, joint->child->odeBodyID);
	else dJointAttach(joint->odeJointID, NULL, joint->child->odeBodyID);

	switch(joint->type) {
		case JT_REVOLUTE:
			dJointSetHingeAnchor(joint->odeJointID, hingePosition.x, hingePosition.y, hingePosition.z);
			break;
		case JT_FIX:
			dJointSetFixed(joint->odeJointID);
			break;
		case JT_UNIVERSAL:
			dJointSetUniversalAnchor(joint->odeJointID, hingePosition.x, hingePosition.y, hingePosition.z);
			break;
		case JT_BALL:
			dJointSetBallAnchor(joint->odeJointID, hingePosition.x, hingePosition.y, hingePosition.z);
			break;
		default:
			break;
	}

	/* set the proper positions where the link should actually be at this time */

	slVectorXform(joint->child->position.rotation, clinkPoint, &childPosition);
	slVectorSub(&hingePosition, &childPosition, &childPosition);

	dBodySetRotation(joint->child->odeBodyID, savedChildR);
	dBodySetPosition(joint->child->odeBodyID, childPosition.x, childPosition.y, childPosition.z);
	slVectorCopy(&childPosition, &joint->child->position.location);

	return 0;
}

/*!
	\brief Creates an empty new multibody struct.
*/

slMultibody *slMultibodyNew(slWorld *w) {
	slMultibody *m;

	m = new slMultibody;

	m->world = w;
	m->handleSelfCollisions = 0;

	m->root = NULL;

	return m;
}

/*!
	\brief Sets the root of the multibody.

	Sets the root of the multibody.  Other links in the multibody are automatically
	computed.
*/

void slMultibodySetRoot(slMultibody *m, slLink *root) {
	m->root = root;
	root->multibody = m;

	slMultibodyUpdate(m);
}

/*!
	\brief Updates the multibody by recomputing the connected links.

	Recomputes the links, the link count and the mass.
*/

void slMultibodyUpdate(slMultibody *m) {
	slList *links;

	if(!m) return;

	m->links.clear();

	links = slLinkList(m->root, NULL, 1);

	while(links) {
		m->links.push_back((slLink*)links->data);
		links = links->next;
	}

	m->linkCount = slMultibodyCountLinks(m);
	m->mass = slMultibodyComputeMass(m);

	if(m->world->initialized) slMultibodyInitCollisionFlags(m, m->world->clipData->pairList);
}

/*!
	\brief Updates the positions and bounding boxes for all links in a multibody.
*/

void slMultibodyUpdatePositions(slMultibody *mb) {
	std::vector<slLink*>::iterator i;

	for(i = mb->links.begin(); i != mb->links.end(); i++ ) {
		slLink *link = *i;

		slLinkUpdatePositions(link);
		slLinkUpdateBoundingBox(link);
	}
}

/*!
	\brief Converts a breve matrix to an ODE matrix.
*/

void slSlToODEMatrix(double m[3][3], dReal *r) {
	r[0] = m[0][0];
	r[1] = m[0][1];
	r[2] = m[0][2];

	r[4] = m[1][0];
	r[5] = m[1][1];
	r[6] = m[1][2];

	r[8] = m[2][0];
	r[9] = m[2][1];
	r[10] = m[2][2];
}

/*!
	\brief Converts an ODE matrix to a breve matrix.
*/

void slODEToSlMatrix(dReal *r, double m[3][3]) {
	m[0][0] = r[0];
	m[0][1] = r[1];
	m[0][2] = r[2];

	m[1][0] = r[4];
	m[1][1] = r[5];
	m[1][2] = r[6];

	m[2][0] = r[8];
	m[2][1] = r[9];
	m[2][2] = r[10];
}

/*!
	\brief Checks this multibody's links for collisions with other links in 
	the multibody.
*/

int slMultibodyCheckSelfPenetration(slWorld *world, slMultibody *m) {
	slVclipData *vc;
	int x, y;
	slPairEntry *pe;
	std::vector<slLink*>::iterator i1;
	std::vector<slLink*>::iterator i2;

	if(!world->initialized) slVclipDataInit(world);
	vc = world->clipData;

	for(i1 = m->links.begin(); i1 != m->links.end(); i1++ ) {
		slLink *link1 = *i1;

		// start with the next thing in the list.  this way we avoid 
		// doing repeats like 1st-2nd and 2nd-1st

		for( i2 = i1 ; i2 != m->links.end(); i2++ ) {
			slLink *link2 = *i2;

			if(link1 != link2) {
				x = link1->clipNumber;
				y = link2->clipNumber;

				if(x > y) pe = &vc->pairList[x][y];
				else pe = &vc->pairList[y][x];

				// printf("flags [%d][%d] %p %x\n", x, y, pe, pe->flags);

				if(pe->flags == BT_ALL && slVclipTestPair(vc, pe, NULL)) return 1;
			}
		}
	}

	return 0;
}

void *slMultibodyGetCallbackData(slMultibody *m) {
	return m->callbackData;
}

void slMultibodySetCallbackData(slMultibody *m, void *c) {
	m->callbackData = c;
}

void slMultibodySetHandleSelfCollisions(slMultibody *m, int n) {
	m->handleSelfCollisions = 1;
}
