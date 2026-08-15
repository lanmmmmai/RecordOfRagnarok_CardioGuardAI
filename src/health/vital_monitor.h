#ifndef VITAL_MONITOR_H
#define VITAL_MONITOR_H

// Threshold alerts on the vital signs.
//
// This runs downstream of the full DSP chain on purpose. Before stage 5 existed
// the displayed heart rate could jump to 180 on an arm swing, and an alert
// built on that would have sent the family a cardiac emergency every time the
// wearer waved. A wrong medical alert is worse than no alert: it teaches people
// to ignore the next one.

void initVitalMonitor();
void updateVitalMonitor();

#endif  // VITAL_MONITOR_H
