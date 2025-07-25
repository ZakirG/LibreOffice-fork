/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/bootstrapfixture.hxx>

#include <vcl/audioplayer.hxx>
#include <vcl/wrkwin.hxx>

class AudioPlayerControlTest : public test::BootstrapFixture
{
public:
    AudioPlayerControlTest() : BootstrapFixture(true, false) {}

    void testAudioPlayerControlCreation();
    void testDefaultState();
    void testSetFilename();
    void testSetMediaURL();

    CPPUNIT_TEST_SUITE(AudioPlayerControlTest);
    CPPUNIT_TEST(testAudioPlayerControlCreation);
    CPPUNIT_TEST(testDefaultState);
    CPPUNIT_TEST(testSetFilename);
    CPPUNIT_TEST(testSetMediaURL);
    CPPUNIT_TEST_SUITE_END();
};

void AudioPlayerControlTest::testAudioPlayerControlCreation()
{
    // Test that the AudioPlayerControl can be created successfully
    ScopedVclPtrInstance<WorkWindow> xWindow(nullptr, WB_APP | WB_STDWORK);
    ScopedVclPtrInstance<vcl::AudioPlayerControl> xAudioPlayer(xWindow.get());

    // Verify the control was created
    CPPUNIT_ASSERT(xAudioPlayer.get() != nullptr);
    
    // Check that it's properly a VCL control
    CPPUNIT_ASSERT(dynamic_cast<vcl::Control*>(xAudioPlayer.get()) != nullptr);
    CPPUNIT_ASSERT(dynamic_cast<vcl::Window*>(xAudioPlayer.get()) != nullptr);
}

void AudioPlayerControlTest::testDefaultState()
{
    // Test that the default state is set correctly
    ScopedVclPtrInstance<WorkWindow> xWindow(nullptr, WB_APP | WB_STDWORK);
    ScopedVclPtrInstance<vcl::AudioPlayerControl> xAudioPlayer(xWindow.get());

    // Check default state
    CPPUNIT_ASSERT_EQUAL(OUString(), xAudioPlayer->GetFilename());
    CPPUNIT_ASSERT_EQUAL(OUString(), xAudioPlayer->GetMediaURL());
    CPPUNIT_ASSERT_EQUAL(false, xAudioPlayer->IsPlaying());
    
    // Check default size (should be a wide rectangle)
    Size aOptimalSize = xAudioPlayer->GetOptimalSize();
    CPPUNIT_ASSERT(aOptimalSize.Width() > aOptimalSize.Height()); // Wide rectangle
    CPPUNIT_ASSERT_EQUAL(Size(320, 80), aOptimalSize); // Specific size from implementation
}

void AudioPlayerControlTest::testSetFilename()
{
    // Test setting and getting filename
    ScopedVclPtrInstance<WorkWindow> xWindow(nullptr, WB_APP | WB_STDWORK);
    ScopedVclPtrInstance<vcl::AudioPlayerControl> xAudioPlayer(xWindow.get());

    const OUString sTestFilename("test_audio.mp3");
    xAudioPlayer->SetFilename(sTestFilename);
    
    CPPUNIT_ASSERT_EQUAL(sTestFilename, xAudioPlayer->GetFilename());
    
    // Test setting empty filename
    xAudioPlayer->SetFilename(OUString());
    CPPUNIT_ASSERT_EQUAL(OUString(), xAudioPlayer->GetFilename());
}

void AudioPlayerControlTest::testSetMediaURL()
{
    // Test setting and getting media URL
    ScopedVclPtrInstance<WorkWindow> xWindow(nullptr, WB_APP | WB_STDWORK);
    ScopedVclPtrInstance<vcl::AudioPlayerControl> xAudioPlayer(xWindow.get());

    const OUString sTestURL("file:///path/to/test_audio.mp3");
    xAudioPlayer->SetMediaURL(sTestURL);
    
    CPPUNIT_ASSERT_EQUAL(sTestURL, xAudioPlayer->GetMediaURL());
    
    // Setting a URL should not automatically start playing
    CPPUNIT_ASSERT_EQUAL(false, xAudioPlayer->IsPlaying());
    
    // Test setting empty URL
    xAudioPlayer->SetMediaURL(OUString());
    CPPUNIT_ASSERT_EQUAL(OUString(), xAudioPlayer->GetMediaURL());
}

CPPUNIT_TEST_SUITE_REGISTRATION(AudioPlayerControlTest);

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 