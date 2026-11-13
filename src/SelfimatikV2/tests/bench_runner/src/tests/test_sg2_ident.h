#pragma once

// Test sg2_ident : identification des bits StallGuard2 du TMC2226.
// Deux entrées : axe Y (test 1) et axe ROT (test 2).
void test_sg2_y_setup();
void test_sg2_y_loop();
void test_sg2_y_teardown();
void test_sg2_rot_setup();
void test_sg2_rot_loop();
void test_sg2_rot_teardown();
// Home Y seul : amène Y au switch (haut) et maintient le moteur alimenté
// (requis avant le test ROT : Y doit être en haut). Taper 'y'.
void test_home_y_setup();
void test_home_y_loop();
void test_home_y_teardown();

// Déclaré dans runner.cpp : termine proprement le test actif.
void finishTest();
