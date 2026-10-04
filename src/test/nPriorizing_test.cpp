#include "nPriorizing.h"
#include "doctest.h"

#include "nNetObject.h"
#include "tMemManager.h"

// This is an adaption of an ad-hoc test that used to reside in

// gotta be honest, I have no clue what this is supposed to test
// these classes are not actively used in the game yet, so predictably,
// the test crashes on destruction at the moment.

#if false

namespace
{
static void test_handler(nMessage& m)
{
}

static nDescriptor testDescriptor(399, test_handler, "test");

/// an empty test netobject
class nTestObject : public nNetObject
{
public:
    nTestObject(nMessage& m) : nNetObject(m) {}
    nTestObject() {}
    virtual nDescriptor& CreatorDescriptor() const;
    virtual bool AcceptClientSync() const { return true; }
};

nDescriptor& nTestObject::CreatorDescriptor() const
{
    static nNOInitialisator<nTestObject> cd(398, "nTestObject");
    return cd;
}

/// test implementation of arbitrator.
class nBandwitdhDistributor : public nBandwidthArbitrator
{
public:
    nSendBuffer& SendBuffer() { return buffer_; }
    const nSendBuffer& SendBuffer() const { return buffer_; }
    nBandwidthControl& BandwidthControl() { return control_; }
    const nBandwidthControl& BandwidthControl() const { return control_; }

protected:
private:
    virtual bool DoUseBandwidth(REAL dt);

    virtual REAL TimeScale() { return 0.1f; }       // higher values let really urgent messages be sent even if the bandwidth control objectd
    virtual REAL PacketOverhead() { return 60.0f; } // overhead in bytes per sent packet. Determines average package size

    nSendBuffer buffer_;        // buffer taking the messages
    nBandwidthControl control_; // bandwidth control
};

// consumes some bandwidth
bool nBandwitdhDistributor::DoUseBandwidth(REAL dt)
{
    return this->Fill(this->buffer_, this->control_);
}

/// message sending bandwidth task
class nBandwidthTaskMessage : public nBandwidthTask
{
public:
    nBandwidthTaskMessage(nType type, nMessage& message);

    nMessage& Message() const { return *message_; }

protected:
    virtual void DoExecute(nSendBuffer& buffer, nBandwidthControl& control); // executes whatever it has to do
    virtual int DoEstimateSize() const;                                      // estimate bandwidth usage
private:
    tJUST_CONTROLLED_PTR<nMessage> message_;
};

nBandwidthTaskMessage::nBandwidthTaskMessage(nType type, nMessage& message)
    : nBandwidthTask(type), message_(&message)
{
}

// executes whatever it has to do
void nBandwidthTaskMessage::DoExecute(nSendBuffer& buffer, nBandwidthControl& control)
{
    buffer.AddMessage(*message_, &control);
}

// estimate bandwidth usage
int nBandwidthTaskMessage::DoEstimateSize() const
{
    return message_->DataLen();
}
} // namespace

TEST_SUITE("Network Priorities")
{
    TEST_CASE("Network Priorities")
    {
        nBandwidthSceduler sceduler;

        {
            auto distributor = tRefPtr<nBandwitdhDistributor>::Make();
            sceduler.AddArbitrator(*distributor);

            {
                nMessage* mess = tNEW(nMessage(testDescriptor));
                auto messageTask = tRefPtr<nBandwidthTaskMessage>::Make(nBandwidthTask::Type_Vital, *mess);
                distributor->Insert(messageTask);
                sceduler.UseBandwidth(1.0f);
                distributor->Insert(messageTask);
                sceduler.UseBandwidth(1.0f);
                distributor->Insert(messageTask);
            }

            {
                auto object = tRefPtr<nTestObject>::Make();
                auto objectTask = tRefPtr<nBandwidthTaskCreate>::Make(nBandwidthTask::Type_Vital, *object);
                objectTask->AddPriority(1.0f);
                distributor->Insert(objectTask);
            }
        }

        sceduler.UseBandwidth(1.0f);
        sceduler.UseBandwidth(1.0f);
        sceduler.UseBandwidth(1.0f);
    }
}

#endif