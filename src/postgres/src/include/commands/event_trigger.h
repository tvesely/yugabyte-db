/*-------------------------------------------------------------------------
 *
 * event_trigger.h
 *	  Declarations for command trigger handling.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/commands/event_trigger.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef EVENT_TRIGGER_H
#define EVENT_TRIGGER_H

#include "catalog/pg_event_trigger.h"
#include "nodes/parsenodes.h"

typedef struct EventTriggerData
{
	NodeTag		type;
	char	   *event;				/* event name */
	Node	   *parsetree;			/* parse tree */
	const char *tag;				/* command tag */
} EventTriggerData;

/*
 * EventTriggerData is the node type that is passed as fmgr "context" info
 * when a function is called by the event trigger manager.
 */
#define CALLED_AS_EVENT_TRIGGER(fcinfo) \
	((fcinfo)->context != NULL && IsA((fcinfo)->context, EventTriggerData))

extern void CreateEventTrigger(CreateEventTrigStmt *stmt);
extern void RemoveEventTriggerById(Oid ctrigOid);
extern Oid	get_event_trigger_oid(const char *trigname, bool missing_ok);

extern void AlterEventTrigger(AlterEventTrigStmt *stmt);
extern void RenameEventTrigger(const char* trigname, const char *newname);
extern void AlterEventTriggerOwner(const char *name, Oid newOwnerId);
extern void AlterEventTriggerOwner_oid(Oid, Oid newOwnerId);

extern bool EventTriggerSupportsObjectType(ObjectType obtype);
extern void EventTriggerDDLCommandStart(Node *parsetree);

#endif   /* EVENT_TRIGGER_H */
