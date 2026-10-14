#pragma once

// Test sg2_ident : identification des bits StallGuard2 du TMC2226.
void test_sg2_ident_setup();
void test_sg2_ident_loop();
void test_sg2_ident_teardown();

// Déclaré dans runner.cpp : termine proprement le test actif.
void finishTest();
