#ifndef APPLICATION_H
#define APPLICATION_H

#include <adwaita.h>

#define LOCK_TYPE_APPLICATION (lock_application_get_type())

G_DECLARE_FINAL_TYPE(LockApplication, lock_application, LOCK, APPLICATION, AdwApplication);

LockApplication *lock_application_new(void);

#endif                          // APPLICATION_H
