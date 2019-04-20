/*
 * app.cpp
 *
 * Author: cmd0cd
 */

#include "app.h"

const char *App::LOGTAG = "AppTask";
static StaticQueue_t InCommingQueue;
static uint8_t CommandBuffer[App::QUEUE_SIZE * App::ITEM_SIZE] = { 0 };

App::App() {

}

App::~App() {

}


void App::run(void *data) {

}


void App::onStart() {

}


void App::onStop() {

}

