void AnalyzeSynch() {

	TFile * input = new TFile("synchOutput.root", "READ");

	TTree * tree_read = (TTree*)input->Get("readAccesses");
	uint32_t nEventsRead[2];
    TTimeStamp * readTimeStamp = NULL;
    tree_read->SetBranchAddress("nevents_0", &nEventsRead[0]);
    tree_read->SetBranchAddress("nevents_1", &nEventsRead[1]);
    tree_read->SetBranchAddress("readTimeStamp", &readTimeStamp);

    TTree * tree_write = (TTree*)input->Get("writes");
    TH1D * h0 = NULL;
    TH1D * h1 = NULL;
    double t_ttt0, t_ttt1;
    tree_write->SetBranchAddress("wave_b0_ch1", &h0);
    tree_write->SetBranchAddress("wave_b1_ch1", &h1);
    tree_write->SetBranchAddress("ttt0", &t_ttt0);
    tree_write->SetBranchAddress("ttt1", &t_ttt1);

    tree_write->GetEntry(5);
    h0->Draw();
    h1->Draw("same");

    cout << "writes: ttt0 = " << t_ttt0 << ", ttt1 = " << t_ttt1 << " -- delta = " << t_ttt1 - t_ttt0 << endl;

    uint32_t totalRead0 = 0;
    uint32_t totalRead1 = 0;

    for(int i = 0; i < tree_read->GetEntries(); i++) {
    	tree_read->GetEntry(i);

    	totalRead0 += nEventsRead[0];
    	totalRead1 += nEventsRead[1];

    	if(nEventsRead[0] != nEventsRead[1]) {
    		cout << "Found entry with incomplete read: got ("
    			 << readTimeStamp->AsString() << ") "
    			 << nEventsRead[0] << ", " << nEventsRead[1] << " events -- ";
    		i += 1;
    		if(i >= tree_read->GetEntries()) break;
    		tree_read->GetEntry(i);
    		totalRead0 += nEventsRead[0];
    		totalRead1 += nEventsRead[1];
			cout << "next attempt got ("
				 << readTimeStamp->AsString() << ") "
				 << nEventsRead[0] << ", " << nEventsRead[1] << " events" << endl;
    	}

    }

    cout << endl << "Read a total of " << totalRead0 << ", " << totalRead1 << " events" << endl;

    tree_read->ResetBranchAddresses();

    input->Close();

}