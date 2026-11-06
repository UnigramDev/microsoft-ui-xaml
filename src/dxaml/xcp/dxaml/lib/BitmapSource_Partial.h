// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BitmapSource.g.h"
#include "DXamlAsyncBase.h"
#include <fwd/windows.storage.h>

// Private: not in the metadata, reached by QueryInterface on a BitmapImage or WriteableBitmap.
// SetAtlasGroup places the hardware surfaces the source allocates from then on in a separate set of DComp atlas pages,
// shared only with sources in the same group; 0, the default, is the set all other content uses. Pages are freed only
// when empty, so an app that keeps some images for a long time while others churn can give each kind its own group.
MIDL_INTERFACE("c81653bf-fd64-43aa-bfb4-5006d578301f")
IBitmapSourceAtlasGroup : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SetAtlasGroup(UINT32 atlasGroup) = 0;
};

namespace DirectUI
{
    // A reusable event.  Used to signal that an async process has completed
    // Currently only used in BitmapSource
    // TODO:  Move to own file and generalize beyond BitmapSource usage pattern
    class EventWrapper
    {
        public:
            EventWrapper()
            {
                m_hEvent = NULL;
            }
            ~EventWrapper();

            // Call Start before the operation you want to wait on
            HRESULT Start();
            // Start the async operation and then call Wait
            HRESULT Wait();
            // Call Set when the operation completes
            HRESULT Set();
            // Call Close to clean up resources once you're done waiting
            void Close();
        private:
            HANDLE m_hEvent;
    };



    PARTIAL_CLASS(BitmapSource)
        , public IBitmapSourceAtlasGroup
    {
        friend class BitmapSourceFactory;

    public:
        BitmapSource();
        ~BitmapSource() override;

        IFACEMETHOD(SetAtlasGroup)(UINT32 atlasGroup) override;

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    public:

        _Check_return_ HRESULT SetSourceImpl(_In_ wsts::IRandomAccessStream* pStreamSource);
        _Check_return_ HRESULT SetSourceAsyncImpl(_In_ wsts::IRandomAccessStream* pStreamSource, _Outptr_ wf::IAsyncAction** returnValue);
        _Check_return_ HRESULT SetSourceShared(_In_ wsts::IRandomAccessStream* pStreamSource);

    protected:
        virtual _Check_return_ HRESULT OnStreamReadCompleted(_In_ wsts::IStreamReadOperation *pOperation, _In_ wf::AsyncStatus status);

    private:
        void CancelSetSourceAsyncActions();

    private:
        // Used to assign unique ids to AsyncActions
        static ULONG z_ulUniqueAsyncActionId;

    private:
        ctl::ComPtr<wsts::IStreamReadOperation> m_spStreamReadOperation;
        ctl::ComPtr<wf::IAsyncInfo> m_spStreamReadOperationAsyncInfo;
        EventWrapper m_Event;

        // We can only have one AsyncAction outstanding per BitmapSource.  In some ones the best design
        // would be for BitmapSource to actually implement IAsyncAction, but we have a shared implementation
        // that we can pick up easily.  We create new ones each time we start an Async operation, but they
        // don't get cleaned up immediately when the operation completes.  This is consistent with thinking of BitmapSource
        // as implementing AsyncAction and avoids a complex circular lifetime issue with the action pointing to the BitmapSource
        // and the BitmapSource pointing to the Action.  Since the Action does nothing except maintain the Async state machine
        // and refers to no expensive resources, this is fine for now.
        Microsoft::WRL::ComPtr<BitmapSourceSetSourceAsyncAction> m_spSetSourceAsyncAction;

        // Some implementations of IRandomAccessStream::ReadAsync do not ref the stream they are reading, so
        // we ref it temporarily to keep it from going away out from under us
        ctl::ComPtr<wsts::IRandomAccessStream> m_spStream;

    public:
        static _Check_return_ HRESULT SetSourceAsync(_In_ CBitmapSource* pNative, _In_ wsts::IRandomAccessStream* pStreamSource,
                                                     _Outptr_ wf::IAsyncAction** ppReturnValue);
    };
}
