#pragma once

#include <functional>

#include <uv.h>

class SimpleCallback
{
public:
	std::function<void()> fn;
	SimpleCallback(const std::function<void()>& fn) : fn(fn) {}
};

template<typename T>
class WorkCallback
{
public:
	std::function<T()> fn;
	WorkCallback(const std::function<T()>& fn) : fn(fn) {}
};

class TimerCallback
{
public:
	std::function<void()> fn;
	TimerCallback(const std::function<void()>& fn) : fn(fn) {}
};

template<typename T>
class AfterWorkCallback
{
public:
	std::function<void(T&)> fn;
	AfterWorkCallback(const std::function<void(T)>& fn) : fn(fn) {}
};

static inline void uv_close_free(void* handle)
{
	uv_close((uv_handle_t*)handle, [](uv_handle_t* handleInner) {
		delete handleInner;
	});
}

template<typename T>
struct WorkDataResult
{
	WorkCallback<T> workCb;
	AfterWorkCallback<T> aftetWorkCb;
	T result;

	WorkDataResult(const WorkCallback<T>& workCb, const AfterWorkCallback<T>& aftetWorkCb)
					: workCb(workCb), aftetWorkCb(aftetWorkCb) {}
};

struct WorkDataVoid
{
	SimpleCallback workCb;
	SimpleCallback aftetWorkCb;

	WorkDataVoid(const SimpleCallback& workCb, const SimpleCallback& aftetWorkCb)
					: workCb(workCb), aftetWorkCb(aftetWorkCb) {}
};

class UVPP
{
	uv_loop_t* loop;

public:
	UVPP()
	{
		loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
		uv_loop_init(loop);
	}

	~UVPP()
	{
		uv_loop_close(loop);
		free(loop);
	}

	int run(uv_run_mode mode)
	{
		return uv_run(loop, mode);
	}

	void queueWork(const std::function<void()>& workCb, const std::function<void()>& afterWorkCb)
	{
		uv_work_t* handle = new uv_work_t;
		handle->data = new WorkDataVoid(workCb, afterWorkCb);

		uv_queue_work(loop, handle,
		              [](uv_work_t* handleInner) {
			              WorkDataVoid* data = (WorkDataVoid*)handleInner->data;
			              data->workCb.fn();
		              },
		              [](uv_work_t* handleInner, int status) {
			              WorkDataVoid* data = (WorkDataVoid*)handleInner->data;
			              data->aftetWorkCb.fn();
			              delete data;
			              delete handleInner;
		              });
	}

	void queueWork(const std::function<void()>& workCb)
	{
		queueWork(workCb, []() {});
	}

	template<typename T>
	void queueWork(const std::function<T()>& workCb, const std::function<void(T)>& afterWorkCb)
	{
		uv_work_t* handle = new uv_work_t;
		handle->data = new WorkDataResult<T>(workCb, afterWorkCb);

		uv_queue_work(loop, handle,
		              [](uv_work_t* handleInner) {
			              WorkDataResult<T>* data = (WorkDataResult<T>*)handleInner->data;
			              data->result = data->workCb.fn();
		              },
		              [](uv_work_t* handleInner, int status) {
			              WorkDataResult<T>* data = (WorkDataResult<T>*)handleInner->data;
			              data->aftetWorkCb.fn(data->result);
			              delete data;
			              delete handleInner;
		              });
	}

	void asyncSend(const std::function<void()>& cb)
	{
		uv_async_t* handle = new uv_async_t;
		handle->data = new SimpleCallback(cb);

		uv_async_init(loop, handle, [](uv_async_t* handleInner) {
			SimpleCallback* data = (SimpleCallback*)handleInner->data;
			data->fn();
			delete data;
			uv_close_free(handleInner);
		});
		uv_async_send(handle);
	}

	void setTimeout(int timeoutMs, const std::function<void()>& cb)
	{
		uv_timer_t* handle = new uv_timer_t;
		handle->data = new TimerCallback(cb);

		uv_timer_init(loop, handle);
		uv_timer_start(handle,
		               [](uv_timer_t* handleInner) {
			               TimerCallback* data = (TimerCallback*)handleInner->data;
			               data->fn();
			               uv_timer_stop(handleInner);
			               delete data;
			               delete handleInner;
		               },
		               timeoutMs, 0);
	}

	void setInterval(int timeoutMs, const std::function<void()>& cb)
	{
		uv_timer_t* handle = new uv_timer_t;
		handle->data = new TimerCallback(cb);

		uv_timer_init(loop, handle);
		uv_timer_start(handle,
		               [](uv_timer_t* handleInner) {
			               TimerCallback* data = (TimerCallback*)handleInner->data;
			               data->fn();
		               },
		               timeoutMs, timeoutMs);
	}
};