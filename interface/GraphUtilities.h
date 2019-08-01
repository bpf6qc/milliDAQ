#ifndef GRAPH_UTILITES_H
#define GRAPH_UTILITES_H

#include "TGraphErrors.h"

class Efficiency {
 public:
 Efficiency() : nPass(0), nTotal(0), slowNPass(0), slowNTotal(0) {};
  virtual ~Efficiency() {
    if(graph) delete graph;
  };

  Efficiency& operator+=(const bool& passes) {
    if(passes) {
      this->nPass++;
      this->slowNPass++;
    }
    this->nTotal++;
    this->slowNTotal++;
    return *this;
  };

  void Init(const TString title, const int color) {
    graph = new TGraphErrors();
    graph->SetTitle(title);
    graph->SetName(title);
    graph->SetLineColor(color);
    graph->SetMarkerColor(color);
  };

  void ResetSlow() {
    slowNPass = 0;
    slowNTotal = 0;
  };

  void ResetGraphs() {
    const TString title = graph->GetTitle();
    const int color = graph->GetLineColor();

    graph = new TGraphErrors();
    graph->SetTitle(title);
    graph->SetName(title);
    graph->SetLineColor(color);
    graph->SetMarkerColor(color);
  };

  void AddPoint(const int nMillisecondsLive) {
    int n = graph->GetN();

    double endpoint = (n == 0) ? 0.0 : graph->GetX()[n-1] + graph->GetErrorX(n-1);

    double x = endpoint + nMillisecondsLive / 1000. / 2;
    double xerr = nMillisecondsLive / 1000. / 2.;

    graph->SetPoint(n, x, (nTotal > 0) ? (double)nPass / nTotal * 100.0 : -1.0);
    graph->SetPointError(n, xerr, (nTotal > 0) ? std::sqrt((double)nPass) / nTotal * 100.0 : -1.0);

    nPass = 0;
    nTotal = 0;
  };

  double GetValue() { return (nTotal > 0) ? (double)nPass/nTotal : 0.0; };

  unsigned int nPass, nTotal;
  unsigned int slowNPass, slowNTotal;

  TGraphErrors * graph;
};

class Average {
 public:
 Average() : 
  sum(0), sum2(0), nMeasurements(0), 
    slowSum(0), slowSum2(0), slowNMeasurements(0),
    maxMeasurement(0), slowMaxMeasurement(0) {};

  virtual ~Average() { 
    if(graph) delete graph;
    if(maxGraph) delete maxGraph;
  };

  Average& operator+=(const unsigned int& measurement) {
    this->sum += measurement;
    this->sum2 += measurement * measurement;
    this->nMeasurements += 1;
    this->slowSum += measurement;
    this->slowSum2 += measurement * measurement;
    this->slowNMeasurements += 1;
    if(measurement > this->maxMeasurement) this->maxMeasurement = measurement;
    if(measurement > this->slowMaxMeasurement) this->slowMaxMeasurement = measurement;

    return *this;
  };

  void Init(const TString title, const int color, const int colorMax) {
    graph = new TGraphErrors();
    graph->SetTitle(title);
    graph->SetName(title);
    graph->SetLineColor(color);
    graph->SetMarkerColor(color);

    maxGraph = new TGraphErrors();
    maxGraph->SetTitle(title + "_max");
    maxGraph->SetName(title + "_max");
    maxGraph->SetLineColor(colorMax);
    maxGraph->SetMarkerColor(colorMax);
  };

  void ResetSlow() {
    slowSum = 0;
    slowSum2 = 0;
    slowNMeasurements = 0;
    slowMaxMeasurement = 0;
  };

  void ResetGraphs() {
    const TString title = graph->GetTitle();
    const int color = graph->GetLineColor();

    graph = new TGraphErrors();
    graph->SetTitle(title);
    graph->SetName(title);
    graph->SetLineColor(color);
    graph->SetMarkerColor(color);

    const TString titleMax = maxGraph->GetTitle();
    const int colorMax = maxGraph->GetLineColor();

    maxGraph = new TGraphErrors();
    maxGraph->SetTitle(titleMax);
    maxGraph->SetName(titleMax);
    maxGraph->SetLineColor(colorMax);
    maxGraph->SetMarkerColor(colorMax);
  };

  void AddPoint(const int nMillisecondsLive) {
    int n = graph->GetN();

    double endpoint = (n == 0) ? 0.0 : graph->GetX()[n-1] + graph->GetErrorX(n-1);

    double x = endpoint + nMillisecondsLive / 1000. / 2;
    double xerr = nMillisecondsLive / 1000. / 2.;

    if(nMeasurements == 0) {
      graph->SetPoint(n, x, 0);
      graph->SetPointError(n, xerr, 0);

      maxGraph->SetPoint(n, x, maxMeasurement);
      maxGraph->SetPointError(n, xerr, 0.1);
    }
    else {
      graph->SetPoint(n, x, (nMeasurements > 0) ? sum / nMeasurements : 0.0);
      graph->SetPointError(n, xerr, (nMeasurements > 0) ? std::sqrt(sum2) / nMeasurements : 0.0);

      maxGraph->SetPoint(n, x, maxMeasurement);
      maxGraph->SetPointError(n, xerr, 0.1);
    }

    sum = 0;
    sum2 = 0;
    nMeasurements = 0;

    maxMeasurement = 0;
  };

  double GetValue() { return (nMeasurements > 0) ? sum/nMeasurements : 0.0; };

  double sum, sum2;
  unsigned int nMeasurements;

  double slowSum, slowSum2;
  unsigned int slowNMeasurements;

  double maxMeasurement;
  double slowMaxMeasurement;

  TGraphErrors * graph;
  TGraphErrors * maxGraph;
};

class Rate {
 public:
 Rate() : count(0), slowCount(0) {};
  virtual ~Rate() { if(graph) delete graph; };

  Rate& operator++() {
    this->count++;
    this->slowCount++;
    return *this;
  };

  Rate& operator+=(const unsigned int& n) {
    this->count += n;
    this->slowCount += n;
    return *this;
  };

  Rate& operator=(const unsigned int& n) {
    this->count = n;
    this->slowCount = n;
    return *this;
  }

  void Init(const TString title, const int color) {
    graph = new TGraphErrors();
    graph->SetTitle(title);
    graph->SetName(title);
    graph->SetLineColor(color);
    graph->SetMarkerColor(color);
  };

  void ResetSlow() {
    slowCount = 0;
  };

  void ResetGraph() {
    const TString title = graph->GetTitle();
    const int color = graph->GetLineColor();

    graph = new TGraphErrors();
    graph->SetTitle(title);
    graph->SetName(title);
    graph->SetLineColor(color);
    graph->SetMarkerColor(color);
  };

  void AddPoint(const int nMillisecondsLive) {
    if(nMillisecondsLive <= 0) return;

    int n = graph->GetN();
    double endpoint = (n == 0) ? 0.0 : graph->GetX()[n-1] + graph->GetErrorX(n-1);

    graph->SetPoint(n, endpoint + nMillisecondsLive / 1000. / 2, (float)count / nMillisecondsLive * 1000.);
    graph->SetPointError(n, nMillisecondsLive / 1000. / 2., std::sqrt((float)count) / nMillisecondsLive * 1000.);

    count = 0;
  };

  void AddPoint(const int nMillisecondsLive, const float value) {
    int n = graph->GetN();
    double endpoint = (n == 0) ? 0.0 : graph->GetX()[n-1] + graph->GetErrorX(n-1);

    graph->SetPoint(n, endpoint + nMillisecondsLive / 1000. / 2, value);
    graph->SetPointError(n, nMillisecondsLive / 1000. / 2., 1.0);

    count = 0;
  };

  unsigned int count; // reset in ~seconds
  unsigned int slowCount; // reset in ~files (10k events)

  TGraphErrors * graph;
};

class VRate {
 public:
  VRate() {};
  virtual ~VRate() {
    for(auto &v : rates) v.clear();
    rates.clear();
  };

  void Init(const TString title) {
    rates.clear();
    for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
      std::vector<Rate*> v;
      for(unsigned int iChannel = 0; iChannel < nChannelsPerDigitizer; iChannel++) {
	v.push_back(new Rate());
	v.back()->Init(title + Form("_b%d_ch%d", iDigitizer, iChannel), colors[iChannel]);
      }
      rates.push_back(v);
    }
  }

  void ResetSlow() {
    for(auto &v : rates) {
      for(auto &r : v) r->ResetSlow();
    }
  };

  void ResetGraphs() {
    for(auto &v : rates) {
      for(auto &r : v) r->ResetGraph();
    }
  };

  VRate& operator++() {
    for(auto &v : this->rates) {
      for(auto &r : v) {
	r->count++;
	r->slowCount++;
      }
    }
    return *this;
  };

  VRate& operator+=(const unsigned int& n) {
    for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
      for(unsigned int iChannel = 0; iChannel < nChannelsPerDigitizer; iChannel++) {
	(*(this->rates[iDigitizer][iChannel])) += n;
      }
    }
    return *this;
  };

  VRate& operator=(const unsigned int& n) {
    for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
      for(unsigned int iChannel = 0; iChannel < nChannelsPerDigitizer; iChannel++) {
	(*(this->rates[iDigitizer][iChannel])) = n;
      }
    }
    return *this;
  };

  void Increment(const int unsigned iDigitizer, const unsigned int iChannel, const unsigned int n = 1) {
    (*rates[iDigitizer][iChannel]) += n;
  };

  void AddPoint(const int nMillisecondsLive) {
    for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
      for(unsigned int iChannel = 0; iChannel < nChannelsPerDigitizer; iChannel++) {
	rates[iDigitizer][iChannel]->AddPoint(nMillisecondsLive);
      }
    }
  };

  std::vector<std::vector<Rate*> > rates;

 private:
  int colors[nChannelsPerDigitizer] = {
    kBlack, kRed,
    kGreen+2, kBlue,
    kOrange-3, kPink-8,
    kAzure+7, kTeal+5,
    kGray+2, kYellow+3,
    kMagenta+1, kSpring+7,
    kPink+2, kOrange+3,
    kBlue-7, kRed-6
  };

};

#endif
