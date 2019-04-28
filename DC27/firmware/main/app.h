/*
 * app.h
 *
 * Author: cmd0cd
 */

#ifndef DC27_APP_H
#define DC27_APP_H
#include <libesp/Task.h>

class App {
public:
	struct AppMsg {
		int dummy;
	};
public:
	static const char *LOGTAG;
	static const int QUEUE_SIZE = 10;
	static const int ITEM_SIZE = sizeof(AppMsg);
public:
	App();
	virtual ~App();
	virtual void run(void *data);
protected:
	virtual void onStart();
	virtual void onStop();
private:
};

#endif /* DC27_APP_H */
