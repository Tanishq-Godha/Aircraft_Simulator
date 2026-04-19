#ifndef MENU_H
#define MENU_H

void drawMenu();
void drawControls();
void drawMapSelect();
void drawPlaneSelect();
void drawModelSettings();
void mouseClick(int button, int state, int x, int y);

// Controller menu support
void handleMenuNav(int d);
void handleMenuSelect();

#endif
