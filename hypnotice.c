#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "hypnotice.h"

HypnoticeEvent *
HypnoticeEvent_new (char name[HYPNOTICEEVENT_MAXLEN])
{
  HypnoticeEvent *e = NULL;
  
  e = malloc(sizeof(*e));
  assert(e != NULL);
  strncpy(e->name, name, HYPNOTICEEVENT_MAXLEN-1);
  e->nbSubscribers = 0;
  e->callbacks = NULL;
  
  return e;
}

void
HypnoticeEvent_addSubscriber (HypnoticeEvent *e, HypnoticeNotice callback)
{
  int i;

  for (i = 0; i < e->nbSubscribers; i++)
    if (e->callbacks[i] == callback) return;
  
  e->nbSubscribers++;
  e->callbacks = realloc(e->callbacks,
                         e->nbSubscribers * sizeof(*(e->callbacks)));
  assert(e->callbacks != NULL);
  e->callbacks[e->nbSubscribers - 1] = callback;
}

void
HypnoticeEvent_free (HypnoticeEvent *e)
{
  free(e->callbacks);
  free(e);
}

Hypnotice *
Hypnotice_get (void *data)
{
  static Hypnotice *h = NULL;
  if (h == NULL) {
    h = malloc(sizeof(*h));
    assert(h != NULL);
    h->locked = 0;
    h->nbEvents = 0;
    h->events = NULL;
    h->data = data;
  }
  return h;
}

void
Hypnotice_setData (void *data)
{
  Hypnotice *h = Hypnotice_get(NULL);
  h->data = data;
}

void
Hypnotice_lock ()
{
  Hypnotice *h = Hypnotice_get(NULL);
  h->locked = 1;
}

void
Hypnotice_unlock (void *data)
{
  Hypnotice *h = Hypnotice_get(NULL);
  h->locked = 0;
}

void
Hypnotice_subscribe (char eventName[HYPNOTICEEVENT_MAXLEN], HypnoticeNotice callback)
{
  Hypnotice *h = Hypnotice_get(NULL);
  HypnoticeEvent *e = NULL;
  int i;
  
  for (i = 0; i < h->nbEvents; i++)
    if (strcmp(h->events[i]->name, eventName) == 0)
      e = h->events[i];
  
  if (e == NULL) {
    h->nbEvents++;
    h->events = realloc(h->events,
                        h->nbEvents * sizeof(*(h->events)));
    assert(h->events != NULL);
    h->events[h->nbEvents - 1] = e = HypnoticeEvent_new(eventName);
  }
  
  HypnoticeEvent_addSubscriber(e, callback);
}

void
Hypnotice_notify (char eventName[HYPNOTICEEVENT_MAXLEN], void *from)
{
  Hypnotice *h = Hypnotice_get(NULL);
  int i, j;

  if (h->locked) return;
  
  for (i = 0; i < h->nbEvents; i++) {
    if (strcmp(h->events[i]->name, eventName) == 0) {
      for (j = 0; j < h->events[i]->nbSubscribers; j++)
        (*(h->events[i]->callbacks[j]))(h->data, from);
      return;
    }
  }
  fprintf(stderr, "[Hypnotice] Warning: event '%s' has no subscribers.\n", eventName);
}

void
Hypnotice_free (Hypnotice **h)
{
  int i;

  for (i = 0; i < (*h)->nbEvents; i++)
    HypnoticeEvent_free((*h)->events[i]);
  free((*h)->events);
  free(*h);
  *h = NULL;
}
