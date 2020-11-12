#ifndef _SPEEX_WEBRTC_TEST_TIMER_H_
#define _SPEEX_WEBRTC_TEST_TIMER_H_

#include <QDebug>

#include <chrono>
#include <stdexcept>
#include <utility>

namespace SpeexWebRTCTest {

namespace _detail {

template <typename... Ts>
struct is_one_of;

template <typename T, typename U>
struct is_one_of<T, U>
{
	static constexpr bool value = std::is_same<T, U>::value;
};

template <typename T, typename U, typename... Ts>
struct is_one_of<T, U, Ts...>
{
	static constexpr bool value = std::is_same<T, U>::value || is_one_of<T, Ts...>::value;
};


template <class Duration,
          typename = typename std::enable_if<is_one_of<Duration,
                                                       std::chrono::hours,
                                                       std::chrono::minutes,
                                                       std::chrono::seconds,
                                                       std::chrono::milliseconds,
                                                       std::chrono::microseconds,
                                                       std::chrono::nanoseconds>::value>::type>
constexpr const char* duration_suffix()
{
	if (std::is_same<Duration, std::chrono::hours>::value)
	{
		return "h";
	}
	else if (std::is_same<Duration, std::chrono::minutes>::value)
	{
		return "min";
	}
	else if (std::is_same<Duration, std::chrono::seconds>::value)
	{
		return "s";
	}
	else if (std::is_same<Duration, std::chrono::milliseconds>::value)
	{
		return "ms";
	}
	else if (std::is_same<Duration, std::chrono::microseconds>::value)
	{
		return "us";
	}
	else if (std::is_same<Duration, std::chrono::nanoseconds>::value)
	{
		return "ns";
	}
	return "";
}

template <int, class DurationIn>
std::string format_impl(DurationIn)
{
	return {};
}

template <int count, class DurationIn, class FirstDuration, class... RestDurations>
std::string format_impl(DurationIn d)
{
	std::string out;

	auto val = std::chrono::duration_cast<FirstDuration>(d);
	if (val.count() != 0)
	{
		out = std::to_string(val.count()) + duration_suffix<FirstDuration>();

		if (sizeof...(RestDurations) > 0)
		{
			out += " " + format_impl<count + 1, DurationIn, RestDurations...>(d - val);
		}
	}
	else
	{
		if (sizeof...(RestDurations) > 0)
		{
			out = format_impl<count, DurationIn, RestDurations...>(d);
		}
		else if (count == 0)
		{
			out = std::to_string(0) + duration_suffix<FirstDuration>();
		}
	}

	return out;
}

} // namespace _detail

template <class... RestDurations, class DurationIn>
std::string format(DurationIn d)
{
	return _detail::format_impl<0, DurationIn, RestDurations...>(d);
}

template <typename... Durations>
class Timer final
{
public:
	explicit Timer(const QDebug& debug, QString name = "") : debug_(debug), name_(std::move(name))
	{
		start_ = std::chrono::high_resolution_clock::now();
	}

	~Timer()
	{
		TimePoint stop = std::chrono::high_resolution_clock::now();
		if (name_.isEmpty())
			debug_.nospace().noquote() << "Timer: " << format<Durations...>(stop - start_).c_str();
		else
			debug_.nospace().noquote()
			    << "Timer [" << name_ << "]: " << format<Durations...>(stop - start_).c_str();
	}

private:
	using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock,
	                                          std::chrono::high_resolution_clock::duration>;
	QDebug debug_;
	QString name_;
	TimePoint start_;
};

using MsTimer = Timer<std::chrono::seconds, std::chrono::milliseconds, std::chrono::microseconds>;

} // namespace SpeexWebRTCTest

#define TIMER(debug) MsTimer timer(debug, __func__);

#endif //_SPEEX_WEBRTC_TEST_TIMER_H_
