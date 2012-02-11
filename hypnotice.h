#ifndef HYPNOTICE_H__
#define HYPNOTICE_H__

#define HYPNOTICEEVENT_MAXLEN 64
typedef void (*HypnoticeNotice)(void *data, void *from);

typedef struct HypnoticeEvent {
  char name[HYPNOTICEEVENT_MAXLEN];
  int nbSubscribers;
  HypnoticeNotice *callbacks;
} HypnoticeEvent;

HypnoticeEvent * HypnoticeEvent_new (char name[HYPNOTICEEVENT_MAXLEN]);
void HypnoticeEvent_addSubscriber (HypnoticeEvent *e, HypnoticeNotice callback);
void HypnoticeEvent_free (HypnoticeEvent *e);

typedef struct Hypnotice {
  int locked;
  int nbEvents;
  HypnoticeEvent **events;
  void *data;
} Hypnotice;

Hypnotice * Hypnotice_get (void *data);
void Hypnotice_lock ();
void Hypnotice_unlock ();
void Hypnotice_setData (void *data);
void Hypnotice_subscribe (char eventName[HYPNOTICEEVENT_MAXLEN], HypnoticeNotice callback);
void Hypnotice_notify (char eventName[HYPNOTICEEVENT_MAXLEN], void *from);
void Hypnotice_free (Hypnotice **h);

#endif /* HYPNOTICE_H__ */
