#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../src/xrandrrutil.h"

using namespace std;
using ::testing::Return;

class MockEdid : public Edid {
public:
    MOCK_CONST_METHOD0(maxCmHoriz, int());

    MOCK_CONST_METHOD0(maxCmVert, int());

    MOCK_CONST_METHOD1(dpiForMode, double(
            const ModeP &mode));

    MOCK_CONST_METHOD1(closestDpiForMode, int(
            const ModeP &mode));
};

TEST(xrandrutil_renderCmd, renderAll) {
    list <DisplP> displs;
    list <ModeP> modes = {make_shared<Mode>(0, 0, 0, 0)};

    DisplP displ1 = make_shared<Displ>("One", Displ::disconnected, modes, ModeP(), ModeP(), PosP(), EdidP());
    displs.push_back(displ1);

    shared_ptr<MockEdid> edid2 = make_shared<MockEdid>();
    ModeP mode2 = make_shared<Mode>(0, 1, 2, 3);
    EXPECT_CALL(*edid2, closestDpiForMode(mode2)).WillOnce(Return(4));
    DisplP displ2 = make_shared<Displ>("Two", Displ::disconnected, modes, ModeP(), ModeP(), PosP(), edid2);
    displ2->setDesiredActive();
    displ2->setDesiredMode(mode2);
    displ2->desiredPos = make_shared<Pos>(5, 6);
    displs.push_back(displ2);

    DisplP displ3 = make_shared<Displ>("Three", Displ::disconnected, modes, ModeP(), ModeP(), PosP(), EdidP());
    displ3->setDesiredActive();
    displ3->desiredPos = make_shared<Pos>(13, 14);
    displs.push_back(displ3);

    DisplP displ4 = make_shared<Displ>("Four", Displ::disconnected, modes, ModeP(), ModeP(), PosP(), EdidP());
    displ4->setDesiredActive();
    displ4->setDesiredMode(make_shared<Mode>(15, 16, 17, 18));
    displs.push_back(displ4);

    DisplP displ5 = make_shared<Displ>("Five", Displ::disconnected, modes, ModeP(), ModeP(), PosP(), EdidP());
    displ5->setDesiredActive();
    displ5->setDesiredMode(make_shared<Mode>(7, 8, 9, 10));
    displ5->desiredPos = make_shared<Pos>(11, 12);
    displs.push_back(displ5);

    Displ::desiredPrimary = displ2;

    const string expected = ""
            "xrandr \\\n --dpi 4 \\\n"
            " --output One --off \\\n"
            " --output Two --mode 1x2 --rate 3 --pos 5x6 --primary \\\n"
            " --output Three --off \\\n"
            " --output Four --off \\\n"
            " --output Five --mode 8x9 --rate 10 --pos 11x12";

    EXPECT_EQ(expected, renderCmd(displs));
}

TEST(xrandrutil_renderCmd, renderNoDpi) {
    list <DisplP> displs;
    list <ModeP> modes = {make_shared<Mode>(0, 0, 0, 0)};

    DisplP displ1 = make_shared<Displ>("One", Displ::disconnected, modes, ModeP(), ModeP(), PosP(), EdidP());
    displs.push_back(displ1);

    Displ::desiredPrimary = displ1;

    const string expected = ""
            "xrandr \\\n --dpi 96 \\\n"
            " --output One --off";

    EXPECT_EQ(expected, renderCmd(displs));
}

TEST(xrandrutil_renderUserInfo, renderAll) {
    ModeP mode1 = make_shared<Mode>(1, 2, 3, 4);
    ModeP mode2 = make_shared<Mode>(5, 6, 7, 8);
    ModeP mode3 = make_shared<Mode>(9, 10, 11, 12);
    PosP pos = make_shared<Pos>(13, 14);
    shared_ptr<MockEdid> edid1 = make_shared<MockEdid>();
    EXPECT_CALL(*edid1, maxCmHoriz()).WillOnce(Return(15));
    EXPECT_CALL(*edid1, maxCmVert()).WillOnce(Return(16));
    shared_ptr<MockEdid> edid3 = make_shared<MockEdid>();
    EXPECT_CALL(*edid3, maxCmHoriz()).WillOnce(Return(17));
    EXPECT_CALL(*edid3, maxCmVert()).WillOnce(Return(18));

    list <DisplP> displs;

    DisplP dis = make_shared<Displ>("dis", Displ::disconnected, list<ModeP>(), ModeP(), ModeP(), PosP(), edid1);
    displs.push_back(dis);

    DisplP con = make_shared<Displ>("con", Displ::connected, list<ModeP>({mode1, mode2}), ModeP(), ModeP(), PosP(), EdidP());
    displs.push_back(con);

    DisplP act = make_shared<Displ>("act", Displ::active, list<ModeP>({mode3, mode2, mode1}), mode2, mode3, pos, edid3);
    displs.push_back(act);

    const string expected = ""
            "dis disconnected 15cmx16cm\n"
            "con connected\n"
            "  !6x7 8Hz\n"
            "   2x3 4Hz\n"
            "act active 6x7+13+14 8Hz 17cmx18cm\n"
            " +!10x11 12Hz\n"
            "*  6x7 8Hz\n"
            "   2x3 4Hz\n"
            "*current +preferred !optimal";

    EXPECT_EQ(expected, renderUserInfo(displs));

}

class MockXrrWrapper : public XrrWrapper {
public:
    MOCK_METHOD1(xOpenDisplay, Display *(_Xconst char*));

    MOCK_METHOD1(defaultScreen, int(Display *));

    MOCK_METHOD1(screenCount, int(Display *));

    MOCK_METHOD2(rootWindow, Window(Display * , int));

    MOCK_METHOD2(xrrGetScreenResources, XRRScreenResources *(Display *, Window));

    MOCK_METHOD3(xrrGetOutputInfo, XRROutputInfo *(Display *, XRRScreenResources *, RROutput));

    MOCK_METHOD3(xrrGetCrtcInfo, XRRCrtcInfo *(Display *, XRRScreenResources *, RRCrtc));
};

class xrandrutil_discoverDispls : public ::testing::Test {
protected:
    void SetUp() override {
    }

    Display *dpy = (Display *) 1;
    int screen = 2;
    Window rootWindow;
    XRRScreenResources screenResources;
};

TEST_F(xrandrutil_discoverDispls, cannotOpenDisplay) {
    MockXrrWrapper xrrWrapper;

    EXPECT_CALL(xrrWrapper, xOpenDisplay(NULL));

    EXPECT_THROW(discoverDispls(&xrrWrapper), runtime_error);
}

TEST_F(xrandrutil_discoverDispls, excessScreens) {
    MockXrrWrapper xrrWrapper;

    EXPECT_CALL(xrrWrapper, xOpenDisplay(NULL)).WillOnce(Return(dpy));
    EXPECT_CALL(xrrWrapper, defaultScreen(dpy)).WillOnce(Return(screen));
    EXPECT_CALL(xrrWrapper, screenCount(dpy)).Times(2).WillRepeatedly(Return(1));

    EXPECT_THROW(discoverDispls(&xrrWrapper), runtime_error);
}

TEST_F(xrandrutil_discoverDispls, noDisplays) {
    MockXrrWrapper xrrWrapper;

    EXPECT_CALL(xrrWrapper, xOpenDisplay(NULL)).WillOnce(Return(dpy));
    EXPECT_CALL(xrrWrapper, defaultScreen(dpy)).WillOnce(Return(screen));
    EXPECT_CALL(xrrWrapper, screenCount(dpy)).WillOnce(Return(3));
    EXPECT_CALL(xrrWrapper, rootWindow(dpy, screen)).WillOnce(Return(rootWindow));
    EXPECT_CALL(xrrWrapper, xrrGetScreenResources(dpy, rootWindow)).WillOnce(Return(&screenResources));

    screenResources.noutput = 0;

    const list <DisplP> displs = discoverDispls(&xrrWrapper);

    EXPECT_TRUE(displs.empty());
}

// TODO: more tests!