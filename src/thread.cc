/*
** © 2014 by Philipp Dunkel <pip@pipobscure.com>
** Licensed under MIT License.
*/

void FSEvents::threadStart() {
  if (threadloop) return;
  pthread_create(&thread, NULL, &FSEvents::threadRun, this);
}

void HandleStreamEvents(ConstFSEventStreamRef stream, void *ctx, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[]) {
  FSEvents * fse = (FSEvents *)ctx;
  size_t idx;
  fse->lock();
  fse_event event;
  for (idx=0; idx < numEvents; idx++) {
    event.path = (CFStringRef)CFArrayGetValueAtIndex((CFArrayRef)eventPaths, idx);
    event.flags = eventFlags[idx];
    event.id = eventIds[idx];
    CFArrayAppendValue(fse->events, &event);
  }
  fse->asyncTrigger();
  fse->unlock();
}

void *FSEvents::threadRun(void *ctx) {
  FSEvents *fse = (FSEvents*)ctx;
  FSEventStreamContext context = { 0, ctx, NULL, NULL, NULL };
  fse->threadloop = CFRunLoopGetCurrent();
  FSEventStreamRef stream = FSEventStreamCreate(NULL, &HandleStreamEvents, &context, fse->paths, kFSEventStreamEventIdSinceNow, (CFAbsoluteTime) 0.1, kFSEventStreamCreateFlagNone | kFSEventStreamCreateFlagWatchRoot | kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagUseCFTypes);
  FSEventStreamScheduleWithRunLoop(stream, fse->threadloop, kCFRunLoopDefaultMode);
  FSEventStreamStart(stream);
  CFRunLoopRun();
  FSEventStreamStop(stream);
  FSEventStreamUnscheduleFromRunLoop(stream, fse->threadloop, kCFRunLoopDefaultMode);
  FSEventStreamInvalidate(stream);
  FSEventStreamRelease(stream);
  fse->threadloop = NULL;
  return NULL;
}

void FSEvents::threadStop() {
  if (!threadloop) return;
  CFRunLoopStop(threadloop);
  pthread_join(thread, NULL);
}
