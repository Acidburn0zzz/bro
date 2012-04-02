
#ifndef THREADING_MSGTHREAD_H
#define THREADING_MSGTHREAD_H

#include <pthread.h>

#include "DebugLogger.h"

#include "BasicThread.h"
#include "Queue.h"

namespace threading {

class BasicInputMessage;
class BasicOutputMessage;
class HeartbeatMessage;

/**
 * A specialized thread that provides bi-directional message passing between
 * Bro's main thread and the child thread. Messages are instances of
 * BasicInputMessage and BasicOutputMessage for message sent \a to the child
 * thread and received \a from the child thread, respectively.
 *
 * The thread's Run() method implements main loop that processes incoming
 * messages until Terminating() indicates that execution should stop. Once
 * that happens, the thread stops accepting any new messages, finishes
 * processes all remaining ones still in the queue, and then exits.
 */
class MsgThread : public BasicThread
{
public:
	/**
	 * Constructor. It automatically registers the thread with the
	 * threading::Manager.
	 *
	 * Only Bro's main thread may instantiate a new thread.
	 */
	MsgThread();

	/**
	 * Sends a message to the child thread. The message will be proceesed
	 * once the thread has retrieved it from its incoming queue.
	 *
	 * Only the main thread may call this method.
	 *
	 * @param msg The message.
	 */
	void SendIn(BasicInputMessage* msg)	{ return SendIn(msg, false); }

	/**
	 * Sends a message from the child thread to the main thread.
	 *
	 * Only the child thread may call this method.
	 *
	 * @param msg The mesasge.
	 */
	void SendOut(BasicOutputMessage* msg)	{ return SendOut(msg, false); }

	/**
	 * Reports an informational message from the child thread. The main
	 * thread will pass this to the Reporter once received.
	 *
	 * Only the child thread may call this method.
	 *
	 * @param msg  The message. It will be prefixed with the thread's name.
	 */
	void Info(const char* msg);

	/**
	 * Reports a warning from the child thread that may indicate a
	 * problem. The main thread will pass this to the Reporter once
	 * received.
	 *
	 * Only the child thread may call this method.
	 *
	 * @param msg  The message. It will be prefixed with the thread's name.
	 */
	void Warning(const char* msg);

	/**
	 * Reports a non-fatal error from the child thread. The main thread
	 * will pass this to the Reporter once received. Processing proceeds
	 * normally after the error has been reported.
	 *
	 * Only the child thread may call this method.
	 *
	 * @param msg  The message. It will be prefixed with the thread's name.
	 */
	void Error(const char* msg);

	/**
	 * Reports a fatal error from the child thread. The main thread will
	 * pass this to the Reporter once received. Bro will terminate after
	 * the message has been reported.
	 *
	 * Only the child thread may call this method.
	 *
	 * @param msg  The message. It will be prefixed with the thread's name.
	 */
	void FatalError(const char* msg);

	/**
	 * Reports a fatal error from the child thread. The main thread will
	 * pass this to the Reporter once received. Bro will terminate with a
	 * core dump after the message has been reported.
	 *
	 * Only the child thread may call this method.
	 *
	 * @param msg  The message. It will be prefixed with the thread's name.
	 */
	void FatalErrorWithCore(const char* msg);

	/**
	 * Reports a potential internal problem from the child thread. The
	 * main thread will pass this to the Reporter once received. Bro will
	 * continue normally.
	 *
	 * Only the child thread may call this method.
	 *
	 * @param msg  The message. It will be prefixed with the thread's name.
	 */
	void InternalWarning(const char* msg);

	/**
	 * Reports an internal program error from the child thread. The main
	 * thread will pass this to the Reporter once received. Bro will
	 * terminate with a core dump after the message has been reported.
	 *
	 * Only the child thread may call this method.
	 *
	 * @param msg  The message. It will be prefixed with the thread's name.
	 */
	void InternalError(const char* msg);

#ifdef DEBUG
	/**
	 * Records a debug message for the given stream from the child
	 * thread. The main thread will pass this to the DebugLogger once
	 * received.
	 *
	 * Only the child thread may call this method.
	 *
	 * @param msg  The message. It will be prefixed with the thread's name.
	 */
	void Debug(DebugStream stream, const char* msg);
#endif

	/**
	 * Statistics about inter-thread communication.
	 */
	struct Stats
		{
		uint64_t sent_in;	//! Number of messages sent to the child thread.
		uint64_t sent_out;	//! Number of messages sent from the child thread to the main thread
		uint64_t pending_in;	//! Number of messages sent to the child but not yet processed.
		uint64_t pending_out;	//! Number of messages sent from the child but not yet processed by the main thread.

		/// Statistics from our queues.
		Queue<BasicInputMessage *>::Stats  queue_in_stats;
		Queue<BasicOutputMessage *>::Stats queue_out_stats;
		};

	/**
	 * Returns statistics about the inter-thread communication.
	 *
	 * @param stats A pointer to a structure that will be filled with
	 * current numbers.
	 */
	void GetStats(Stats* stats);

protected:
	friend class Manager;
	friend class HeartbeatMessage;

	/**
	 * Pops a message sent by the child from the child-to-main queue.
	 *
	 * This is method is called regularly by the threading::Manager.
	 *
	 * @return The message, wth ownership passed to caller. Returns null
	 * if the queue is empty.
	 */
	BasicOutputMessage* RetrieveOut();

	/**
	 * Triggers a heartbeat message being sent to the client thread.
	 *
	 * This is method is called regularly by the threading::Manager.
	 *
	 * Can be overriden in derived classed to hook into the heart beat,
	 * but must call the parent implementation. Note that this method is
	 * always called by the main thread and must not access data of the
	 * child thread directly. See DoHeartbeat() if you want to do
	 * something on the child-side.
	 */
	virtual void Heartbeat();

	/**
	 * Overriden from BasicThread.
	 *
	 */
	virtual void Run();
	virtual void OnStop();

	/**
	 * Regulatly triggered for execution in the child thread.
	 *
	 * When overriding, one must call the parent class' implementation.
	 *
	 * network_time: The network_time when the heartbeat was trigger by
	 * the main thread.
	 *
	 * current_time: Wall clock when the heartbeat was trigger by the
	 * main thread.
	 */
	virtual bool DoHeartbeat(double network_time, double current_time);

private:
	/**
	 * Pops a message sent by the main thread from the main-to-chold
	 * queue.
	 *
	 * Must only be called by the child thread.
	 *
	 * @return The message, wth ownership passed to caller. Returns null
	 * if the queue is empty.
	 */
	BasicInputMessage* RetrieveIn();

	/**
	 * Queues a message for the child.
	 *
	 * Must only be called by the main thread.
	 *
	 * @param msg  The message.
	 *
	 * @param force: If true, the message will be queued even when we're already
	 * Terminating(). Normally, the message would be discarded in that
	 * case.
	 */
	void SendIn(BasicInputMessage* msg, bool force);

	/**
	 * Queues a message for the main thread.
	 *
	 * Must only be called by the child thread.
	 *
	 * @param msg  The message.
	 *
	 * @param force: If true, the message will be queued even when we're already
	 * Terminating(). Normally, the message would be discarded in that
	 * case.
	 */
	void SendOut(BasicOutputMessage* msg, bool force);

	/**
	 * Returns true if there's at least one message pending for the child
	 * thread.
	 */
	bool HasIn()	{ return queue_in.Ready(); }

	/**
	 * Returns true if there's at least one message pending for the main
	 * thread.
	 */
	bool HasOut()	{ return queue_out.Ready(); }

	/**
	 * Returns true if there might be at least one message pending for the main
	 * thread.
	 */
	bool MightHaveOut() { return queue_out.MaybeReady(); }

	Queue<BasicInputMessage *> queue_in;
	Queue<BasicOutputMessage *> queue_out;

	uint64_t cnt_sent_in;	// Counts message sent to child.
	uint64_t cnt_sent_out;	// Counts message sent by child.
};

/**
 * Base class for all message between Bro's main process and a MsgThread.
 */
class Message
{
public:
	/**
	 * Destructor.
	 */
	virtual ~Message();

	/**
	 * Returns a descriptive name for the message's general type. This is
	 * what's passed into the constructor and used mainly for debugging
	 * purposes.
	 */
	const string& Name() const { return name; }

	/**
	 * Callback that must be overriden for processing a message.
	 */
	virtual bool Process() = 0; // Thread will be terminated if returngin false.

protected:
	/**
	 * Constructor.
	 *
	 * @param arg_name A descriptive name for the type of message. Used
	 * mainly for debugging purposes.
	 */
	Message(const string& arg_name)	{ name = arg_name; }

private:
	string name;
};

/**
 * Base class for messages sent from Bro's main thread to a child MsgThread.
 */
class BasicInputMessage : public Message
{
protected:
	/**
	 * Constructor.
	 *
	 * @param name A descriptive name for the type of message. Used
	 * mainly for debugging purposes.
	 */
	BasicInputMessage(const string& name) : Message(name)	{}
};

/**
 * Base class for messages sent from a child MsgThread to Bro's main thread.
 */
class BasicOutputMessage : public Message
{
protected:
	/**
	 * Constructor.
	 *
	 * @param name A descriptive name for the type of message. Used
	 * mainly for debugging purposes.
	 */
	BasicOutputMessage(const string& name) : Message(name)	{}
};

/**
 * A paremeterized InputMessage that stores a pointer to an argument object.
 * Normally, the objects will be used from the Process() callback.
 */
template<typename O>
class InputMessage : public BasicInputMessage
{
public:
	/**
	 * Returns the objects passed to the constructor.
	 */
	O* Object() const { return object; }

protected:
	/**
	 * Constructor.
	 *
	 * @param name: A descriptive name for the type of message. Used
	 * mainly for debugging purposes.
	 *
	 * @param arg_object: An object to store with the message. 
	 */
	InputMessage(const string& name, O* arg_object) : BasicInputMessage(name)
		{ object = arg_object; }

private:
	O* object;
};

/**
 * A paremeterized OututMessage that stores a pointer to an argument object.
 * Normally, the objects will be used from the Process() callback.
 */
template<typename O>
class OutputMessage : public BasicOutputMessage
{
public:
	/**
	 * Returns the objects passed to the constructor.
	 */
	O* Object() const { return object; }

protected:
	/**
	 * Constructor.
	 *
	 * @param name A descriptive name for the type of message. Used
	 * mainly for debugging purposes.
	 *
	 * @param arg_object An object to store with the message. 
	 */
	OutputMessage(const string& name, O* arg_object) : BasicOutputMessage(name)
		{ object = arg_object; }

private:
	O* object;
};

}


#endif