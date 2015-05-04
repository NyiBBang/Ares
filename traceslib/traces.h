#pragma once

#if !defined(__unix__) && !defined(__linux__) && !defined(__CYGWIN__)
#   define ON_WINDOWS
#endif

#include <iostream>

#ifdef ON_WINDOWS

#include <fstream>
#include <memory>
#include <functional>
#include <vector>
#include <sstream>

namespace traces
{

class AbstractLogger
{
    public:
        virtual ~AbstractLogger() {}
        virtual void log(const std::string& message) = 0;
};

class StreamLogger final : public AbstractLogger
{
    public:
        StreamLogger(std::ostream& os);

    private:
        void log(const std::string& message) override;

        std::ostream& m_os;
};

class CompositeLogger final : public AbstractLogger
{
    public:
        CompositeLogger(std::unique_ptr<AbstractLogger>& coutLogger, std::ostream& os);

    private:
        void log(const std::string& message) override;

        std::unique_ptr<AbstractLogger>& m_coutLogger;
        std::ostream& m_os;
};

class Stream final
{
    public:
        typedef std::ostream& (&stream_manipulator)(std::ostream&);

        Stream(AbstractLogger& logger);
        ~Stream();

        template<class T> Stream& operator<<(T&& t);
        Stream& operator<<(stream_manipulator manip);

    private:
        AbstractLogger& m_logger;
        std::ostringstream m_buffer;
};

class Facade final
{
    public:

#ifdef DEBUG
        static Stream& debug();
#endif
        static Stream& info();
        static Stream& warning();
        static Stream& error();

        static void resetAuxiliaryLogger();
        template<class TAuxiliaryLogger, class... TArgs>
        static void initializeAuxiliaryLogger(TArgs&&... args);

    private:
        Facade();
        static Facade& instance();

        std::unique_ptr<AbstractLogger> m_coutLogger;
        std::ofstream m_fileStream;
        StreamLogger m_noInfoLogger;
        CompositeLogger m_infoLogger;
#ifdef DEBUG
        Stream m_debugStream;
#endif
        Stream m_infoStream;
        Stream m_warnStream;
        Stream m_errorStream;
};

}


template<class T> traces::Stream& traces::Stream::operator<<(T&& t)
{
    m_buffer << std::forward<T>(t);
    return *this;
}

template<class TAuxiliaryLogger, class... TArgs>
void traces::Facade::initializeAuxiliaryLogger(TArgs&&... args)
{
    instance().m_coutLogger.reset(new TAuxiliaryLogger(std::forward<TArgs>(args)...));
}

#ifdef DEBUG
    #define ARES_DEBUG   traces::Facade::debug
#else
    #define ARES_DEBUG() if(false) std::cout
#endif
#define ARES_INFO    traces::Facade::info
#define ARES_WARNING traces::Facade::warning
#define ARES_ERROR   traces::Facade::error

#else // NOT ON WINDOWS
    #define ARES_DEBUG   ARES_ERROR
    #define ARES_INFO    ARES_ERROR
    #define ARES_WARNING ARES_ERROR
    #define ARES_ERROR() if(false) std::cout
#endif