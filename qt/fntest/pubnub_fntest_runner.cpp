/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest.hpp"
#include "pubnub_fntest_basic.hpp"
#include "pubnub_fntest_medium.hpp"

#include <iostream>
#include <functional>

#include <cstdlib>

#include <QCoreApplication>
#include <QTimer>
#include <QThread>


enum class TestResult {
    fail,
    pass,
    indeterminate
};
Q_DECLARE_METATYPE(TestResult);

using TestFN_T = std::function<void(std::string const&,std::string const&, std::string const&)>;

struct TestData {
    TestFN_T pf;
    char const *name;
    TestResult result;
};


#define LIST_TEST(tstname) { tstname, #tstname, TestResult::indeterminate }


static TestData m_aTest[] = {
    LIST_TEST(simple_connect_and_send_over_single_channel),
    LIST_TEST(connect_and_send_over_several_channels_simultaneously),
    LIST_TEST(simple_connect_and_send_over_single_channel_in_group),
    LIST_TEST(connect_and_send_over_several_channels_in_group_simultaneously),
    LIST_TEST(connect_and_send_over_channel_in_group_and_single_channel_simultaneously),
    LIST_TEST(connect_and_send_over_channel_in_group_and_multi_channel_simultaneously),
    LIST_TEST(simple_connect_and_receiver_over_single_channel),
    LIST_TEST(connect_and_receive_over_several_channels_simultaneously),
    LIST_TEST(simple_connect_and_receiver_over_single_channel_in_group),
    LIST_TEST(connect_and_receive_over_several_channels_in_group_simultaneously),
    LIST_TEST(connect_and_receive_over_channel_in_group_and_single_channel_simultaneously),
    LIST_TEST(connect_and_receive_over_channel_in_group_and_multi_channel_simultaneously),
/*	LIST_TEST(broken_connection_test),
	LIST_TEST(broken_connection_test_multi),
	LIST_TEST(broken_connection_test_group),
	LIST_TEST(broken_connection_test_multi_in_group),
	LIST_TEST(broken_connection_test_group_in_group_out),
	LIST_TEST(broken_connection_test_group_multichannel_out),*/
    LIST_TEST(complex_send_and_receive_over_several_channels_simultaneously),
    LIST_TEST(complex_send_and_receive_over_channel_plus_group_simultaneously),
    LIST_TEST(connect_disconnect_and_connect_again),
    LIST_TEST(connect_disconnect_and_connect_again_group),
    LIST_TEST(connect_disconnect_and_connect_again_combo),
//    LIST_TEST(wrong_api_usage),
};

#define TEST_COUNT (sizeof m_aTest / sizeof m_aTest[0])


/** Making this a "thread-agnostic" class, means that we can run the
    tests on main thread or any other thread. Of course, for running
    on another thread, someone needs to create it and "move" us to it.
 */
class TestRunner : public QObject {
    Q_OBJECT
    
    QString d_pubkey;
    QString d_keysub;
    QString d_origin;
    unsigned d_iTest;
    TestFN_T d_pf;
    char const *d_name;
public:
    TestRunner(QString const& pubkey, QString const& keysub, QString const&origin, unsigned iTest, TestFN_T pf, char const *name)
        : d_pubkey(pubkey), d_keysub(keysub), d_origin(origin), d_iTest(iTest), d_pf(pf), d_name(name) {}
    
    unsigned iTest() const { return d_iTest; }

public slots:
    void doTest() {
        TestResult result = TestResult::pass;
        try {
            d_pf(d_pubkey.toStdString(), d_keysub.toStdString(), d_origin.toStdString());
        }
        catch (std::exception &ex) {
            std::cout << "\n\x1b[41m !! " << d_iTest+1 << ". test '" << d_name << "' failed!\nError description: " << ex.what() << "\x1b[m\n" << std::endl;
            result = TestResult::fail;
        }
        emit testFinished(this, result);
    }
    
signals:
    void testFinished(TestRunner *runner, TestResult result);
};


class TestController : public QObject {
    Q_OBJECT

    private:
    QMap<TestRunner*, QThread*> d_runner;

    QString d_pubkey;
    QString d_keysub;
    QString d_origin;
    unsigned d_max_conc_thread;
    TestData *d_test;
    unsigned d_test_count;

    unsigned d_next_test;
    unsigned d_failed_count;
    unsigned d_passed_count;
    unsigned d_indete_count;

public:
    TestController(TestData aTest[], unsigned test_count, QString const& pubkey, QString const& keysub, QString const& origin, unsigned max_conc_thread)
        : d_pubkey(pubkey)
        , d_keysub(keysub)
        , d_origin(origin)
        , d_max_conc_thread(max_conc_thread) 
        , d_test(aTest)
        , d_test_count(test_count) {
    }

public slots:
    void execute() {
        d_next_test = 0;
        d_failed_count = 0;
        d_passed_count = 0;
        d_indete_count = 0;

        nextCycle();
    }

    void onTestFinished(TestRunner *runner, TestResult result) {
        unsigned iTest = runner->iTest();
        d_test[iTest].result = result;
        switch (result) {
        case TestResult::fail:
            ++d_failed_count;
            break;
        case TestResult::pass:
            ++d_passed_count;
            break;
        case TestResult::indeterminate:
            ++d_indete_count;
            std::cout<< "\x1b[33m Indeterminate " << iTest+1 << ". test ('" << d_test[iTest].name << "') of " << d_test_count << "\x1b[m\t";
            /* Should restart the test... */
            break;
        }
        d_runner[runner]->quit();
        d_runner[runner]->wait();
        d_runner.remove(runner);
        ++d_next_test;
        if (d_runner.size() == 0) {
            nextCycle();
        }
    }

private:
    void nextCycle() {
        if (d_next_test >= d_test_count) {
            std::cout << "Test run over." << std::endl;
            if (d_passed_count == d_test_count) {
                std::cout << "\x1b[32m All " << d_test_count << " tests passed\x1b[m" << std::endl;
            }
            else {
                std::cout <<"\x1b[32m " << d_passed_count << " tests passed\x1b[m, \x1b[41m " << d_failed_count << " tests failed!\x1b[m, \x1b[33m " << d_indete_count << " tests indeterminate\x1b[m" << std::endl; 
            }
            QCoreApplication::instance()->quit();
            return;
         }

        unsigned in_this_pass = d_max_conc_thread;
        if (d_next_test + in_this_pass > d_test_count) {
            in_this_pass = d_test_count - d_next_test;
        }
        for (unsigned i = d_next_test; i < d_next_test+in_this_pass; ++i) {
            QThread *pqt = new QThread;
            TestRunner *runner = new TestRunner(d_pubkey, d_keysub, d_origin, i, d_test[i].pf, d_test[i].name);
            runner->moveToThread(pqt);
            connect(pqt, SIGNAL(finished()), runner, SLOT(deleteLater()));
            connect(runner, SIGNAL(testFinished(TestRunner*,TestResult)), this, SLOT(onTestFinished(TestRunner*,TestResult)));
            d_runner[runner] = pqt;
            pqt->start();
            QTimer::singleShot(0, runner, SLOT(doTest()));
        }
    }
};


#include "pubnub_fntest_runner.moc"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    qRegisterMetaType<TestResult>();
    char const *pubkey = (argc > 1) ? argv[1] : "demo";
    char const *keysub = (argc > 2) ? argv[2] : "demo";
    char const *origin = (argc > 3) ? argv[3] : "pubsub.pubnub.com";
    unsigned max_conc_thread = (argc > 4) ? std::atoi(argv[4]) : 1;

    std::cout << "Using: pubkey == " << pubkey << ", keysub == " << keysub << ", origin == " << origin << std::endl;

    TestController ctrl(m_aTest, TEST_COUNT, pubkey, keysub, origin, max_conc_thread);

    QTimer::singleShot(0, &ctrl, SLOT(execute()));
    
    return app.exec();    
}
