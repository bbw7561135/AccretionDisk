#include "stdafx.h"
#include <fstream>
#include <iostream>
//#include <amp_math.h>
#include <algorithm>
//#include <amp.h>
#include <ppl.h>
#include <chrono>
#include <sstream>
#include <vector>
#include <iomanip>      // std::setw

using namespace std;
using namespace :: Concurrency;
//using namespace ::Concurrency::fast_math;

// CONSTANTS*******************************************************************
static const double day = 86400e0;                                          // (day) s
static const double month = 2629743.83e0;                                   // (month) s
static const double year = 31556926e0;                                      // (year) s
static const double M_solar = 1.9891e33;                                    // (solar mass) g
static const double AU = 1.496e13;                                          // (astronomical unit) cm
static const double ly = 9.463e17;                                          // (light year) cm
static const double pc = 3.086e18;                                          // (parsec)cm
static const double c = 2.99792458e10;                                      // (speed of light) cm / s
static const double h = 6.6260755e-27;                                      // (planck's constant) erg s
static const double hbar = 1.05457266e-27;                                  // (reduced planck's constant) erg s
static const double G = 6.67259e-8;                                         // (gravitational constant) cm3 g-1 s-2
static const double e = 4.8032068e-10;                                      // (electron charge) esu
static const double m_e = 9.1093897e-28;                                    // (mass of electron) g
static const double m_p = 1.6726231e-24;                                    // (mass of proton) g
static const double m_n = 1.6749286e-24;                                    // (mass of neutron) g
static const double m_H = 1.6733e-24;                                       // (mass of hydrogen) g
static const double k = 1.380658e-16;                                       // (boltzmann's constant) erg K-1
static const double eV = 1.6021772e-12;                                     // (electron volt) erg
static const double a = 5.67051e-5;                                         // (stefan boltzmann constant) erg cm-2 K-4 s-1
static const double thompson = 6.6524e-25;                                  // (thompson cross-section for electron) cm-2
static const double rad_const = 7.5646e-15;									// (radiation constant) erg cm-3 K-4
static const double PI = 3.14159265358979323846;
//******************************************************************************************************************

// VARIABLES
static double M_compact;								// Mass of the compact object
static double M_disk;									// Mass of the disk
static double M_dot_N_grids;							// Accretion rate
static double R_isco, R_outer, X_isco, X_outer;			// Inner and outer radius
static int N_grids, N_time;								// Number of spatial and temporal grids
static double delta_X;											// Step size
static double delta_T;											// time step size in seconds
static double mu_p_hot = 0.63;							// proton ratio (ertan, 2002)
static double mu_p_cold = 0.87;							// proton ratio
static double T_max;
static double T_L = 99999999999;
static double L_instant = 0, L_BB = 0, L_HEXTE = 0, L_Optical = 0;
static double L_previous = 0;
static double alpha_hot = 0.2; // was 0.1 before
static double alpha_cold = 0.033;
static double opacity[19][88];
static double logR[19] = { -8, -7.5, -7, -6.5, -6, -5.5, -5, -4.5, -4, -3.5, -3, -2.5, -2, -1.5, -1, -0.5, 0, 0.5, 1 };
static double logT[88];
static double L_edd = 0;
static double M_dot_edd = 0;
static double R_g = 0;
bool CoronaFormed = false;
bool MaximumLuminosityReached = false;
double R_Corona, Theta_Corona;
int trunc_radius_index = 0;
bool Diverging = false;
int N_samples = 500;



double OpticalThickness(double SurfaceDensity, double opacity);
double NormalizationGaussian(double Mu, double Sigma, double R_isco, double R_outer, double M_disk);
void WriteGraphData(double* X, double* Y, int length, string filename, bool append); 
void WriteGraphData(vector<double> X, vector<int> Y, double T, int length, string filename, bool append);
double eVtoHz(double eV);
void ExtractSpectrum(double* T, double* X, double delta_X, int N_grids, double minEnergyEV, double maxEnergyEV, double resolutionEV, bool append);
double mu_p(double alpha);
double VISC_C(double alpha, double opacity);
double average(double numbers[], int size); 
double T_critical(double T_irr, double R);
double T_c_max(double T_irr, double R);
double T_c_min(double T_irr, double R);
double irr_effect(double T_irr); 
double BB_Luminosity(vector<double> T_eff, vector<double> X);
double BB_Luminosity(double* T_eff, double* X, int truncation_index); 
double GetLuminosity(vector<double> T, vector<double> X, double minEnergyEV, double maxEnergyEV, double resolutionEV);
int FindHotIndex(vector<double> vAlpha);
double EffectiveTemperature_dubus2014(double E, double R, double V);
double CentralTemperature_dubus2014(double tau, double T_eff, double T_irr);
double E_max(double T_irr, double R);
double E_min(double T_irr, double R); 
void RadiationPressure(double* P_rad, double* T_c);
void GasPressure(double* P_gas, double* E, double* H, double* T_c, double* Alpha); 
void ScaleHeight_FrankKingRaine(double * H, double * E, double * T_c, double* M_dot, double* R, double* Alpha);
double SoundSpeed(double T_c, double alpha);
double TimeDependentCoronaRadius(double T_current, double T_corona, double T_rise, double R_initial, double R_max, bool Formed);
void WriteGraphData(vector<double> X, vector<double> Y, double T, int length, string filename, bool append);
void printmatrix(vector<vector<double>> matrix, int rows, int columns);
void printmatrix(double* matrix, int rows);
double Viscosity(double T_c, double alpha, double H);
void IrradiationTemperature_CentralPointSource(double LUMINOSITY, vector<double> R, vector<double> H, vector<double> &T_irr, bool ShadowEnabled);
void IrradiationTemperature(double LUMINOSITY, double R_corona, double Theta_corona, vector<double> R, vector<double> H, vector<double> &T_irr, bool ShadowEnabled);
void IrradiationTemperature(double LUMINOSITY, double R_corona, vector<double> R, vector<double> H, vector<double> &T_irr, bool ShadowEnabled);
void GetShadows(vector<int> &Shadows, vector<double> R, vector<double> H, double R_corona, double Theta_corona);
void GetShadows(vector<int> &Shadows, vector<double> R, vector<double> H, double R_corona);

void ProduceAnalyticalSolutions(double M_disk, double J_disk);

enum simulation_type {full, thompson_opacity, analytical};
simulation_type s_type = full;

bool EnableIrradiation = false;
bool EnableShadows = false;
bool ProduceAnalytical = false;
bool EnableCoronaFormation = false;
bool EnableCoronaVanishing = false;

int main(int argc, char **argv)
{
	double n; // parameter for input
	if (argc == 1)
	{
		cout << "Accretion disk simulation with parallel CPU computing.\n\n";
		cout << "Please enter the mass of the compact object. (M_solar)\n";
		cin >> n;
		M_compact = n * M_solar;
		cout << "Please enter the mass of the disk. (M_solar)\n";
		cin >> n;
		M_disk = n * M_solar;
		cout << "Please enter the accretion rate. (M_solar s-1)\n";
		cin >> n;
		M_dot_N_grids = n * M_solar;

		R_g = 2 * G * M_compact / pow(c, 2);
	}
	else
	{
		M_compact = stod(argv[1]) * M_solar;
		M_disk = stod(argv[2]) * M_solar;
		M_dot_N_grids = stod(argv[3]) * M_solar;
	}

	// Calculate some initial values **************************************************************************
	//*********************************************************************************************************
	R_isco = 6 * G * M_compact / pow(c, 2);
	R_outer = 5e11; // cm 
	X_isco = sqrt(R_isco);
	X_outer = sqrt(R_outer);
	L_edd = 1.3e38 * (M_compact / M_solar);
	M_dot_edd = L_edd * 2 * R_isco / (G* M_compact);		// Luminosity in ergs/s
	//*********************************************************************************************************
	//*********************************************************************************************************
	
	cout << "Mass of the compact object = " << M_compact << " g.\n";
	cout << "Mass of the disk           = " << M_disk << " g.\n";
	cout << "Innermost stable orbit     = " << R_isco << " cm.\n";
	cout << "Outer radius               = " << R_outer << " cm.\n";
	cout << "Accretion rate             = " << M_dot_N_grids << " g s-1.\n";
	cout << "Eddington Luminosity       = " << L_edd << " erg s-1.\n";
	if (argc == 1)
	{
		cout << "Simulation type? (f, t, a)"; // f = realistic opacities, t = tompson opacities, a = analytical
		char type;
		cin >> type;
		switch (type)
		{
		case 'f':
			s_type = full;
			break;
		case 't':
			s_type = thompson_opacity;
			break;
		case 'a':
			s_type = analytical;
			break;
		}
		cout << "Produce analytical solution? (y, n)";
		cin >> type;
		if (type == 'y')
		{
			ProduceAnalytical = true;
		}
		else
		{
			ProduceAnalytical = false;
		}
		cout << "Enable irradiation? (y, n)";
		cin >> type;
		if (type == 'y')
		{
			EnableIrradiation = true;
			cout << "Enable shadows? (y, n)";
			cin >> type;
			if (type == 'n')
			{
				EnableShadows = false;
			}
			else
			{
				EnableShadows = true;
				cout << "Enable corona formation? (y, n)";
				cin >> type;
				if (type == 'y')
				{
					EnableCoronaFormation = true;

					cout << "Enable corona vanishing? (y, n)";
					cin >> type;
					if (type == 'y')
					{
						EnableCoronaVanishing = true;
					}
					else
						EnableCoronaVanishing = false;
				}
				else
					EnableCoronaFormation = false;
			}
		}
		else
			EnableIrradiation = false;
		cout << "Please enter the number of grids for the radial coordinate.\n";
		cin >> N_grids;
	}
	else
	{
		N_grids = stod(argv[8]);
	}

	if (argc == 1)
	{
		cout << "Please enter the evolution duration. (months)\n";
		cin >> n; T_max = n*month;
	}
	else
	{
		T_max = stod(argv[9])*month;
	}

	cout << "Creating initial conditions...\n";

	//**************************************************************************************************************
	// Calculate steps in X space **********************************************************************************
	delta_X = (X_outer - X_isco) / N_grids;
	//**************************************************************************************************************
	//**************************************************************************************************************

	// Create vectors and variables ********************************************************************************
	//**************************************************************************************************************
	vector<double> E(N_grids, 0);											// Surface mass density
	vector<double> S(N_grids, 0);											// Surface mass density * X
	vector<double> S_new(N_grids, 0);										// new Surface mass density * X
	vector<double> M_dot(N_grids, 0);										// Mass transfer rate
	vector<double> V(N_grids, 0);											// Kinematic viscosity
	vector<double> V_new(N_grids, 0);										// new Kinematic viscosity
	vector<double> T_eff(N_grids, 0);										// Effective temperature
	vector<double> T_irr(N_grids, 0);										// Irradiation temperature
	vector<double> T_c(N_grids, 0);											// Central temperature
	vector<double> H(N_grids, 0);											// Pressure scale height
	vector<double> O(N_grids, 0);											// Realistic opacity
	vector<double> Alpha(N_grids, 0);										// Alpha parameter
	vector<double> R(N_grids, 0);										// Radius
	vector<double> X(N_grids, 0);												// Radius^2
	vector<vector<double>> Coefficient_Matrix;							// Tridiagonal matrix for implicit solution
	Coefficient_Matrix.resize(N_grids, vector<double>(N_grids, 0));
	vector<double> Coefficient_C(N_grids, 0);					// C coefficient for tridiagonal matrix
	vector<double> Coefficient_B(N_grids, 0);					// B coefficient for tridiagonal matrix
	vector<double> Coefficient_A(N_grids, 0);					// A coefficient for tridiagonal matrix
	vector<double> Coefficient_D(N_grids, 0);					// D coefficient for tridiagonal matrix
	vector<double> Coefficient_C_new(N_grids, 0);				// new C coefficient for tridiagonal matrix
	vector<double> Coefficient_D_new(N_grids, 0);				// new D coefficient for tridiagonal matrix
	double J_total = 0;													// Total angular momentum
	double M_total = 0;													// Total mass
	//**************************************************************************************************************
	//**************************************************************************************************************

	// Read opacity table ************************************************************************************************************************************
	//********************************************************************************************************************************************************
	ifstream opal("table_merged.txt");
	if (opal.is_open())
	{
		int row = 0, col = 0;
		while (!opal.eof())
		{
			string line;
			getline(opal, line);
			stringstream ss(line);
			ss >> logT[row];
			col = 0;
			while (ss >> opacity[col][row])
			{
				col++;
			}
			row++;
		}
	}
	else
	{
		cout << "Opacity table not found!\n";
	}
	//********************************************************************************************************************************************************
	//********************************************************************************************************************************************************

	// Create initial conditions******************************************************************************************************************************
	//********************************************************************************************************************************************************
	auto start = chrono::high_resolution_clock::now();

	
	double Mu = 5e10; // cm
	double Sigma = 1e10; // 1e10 previously
	cout << "Maximum of the initial gaussian is at " << Mu << " cm.\n";
	cout << "Sigma of the initial gaussian is " << Sigma << " cm.\n";
	double E_0 = NormalizationGaussian(Mu, Sigma, R_isco, R_outer, M_disk);

	parallel_for(0, N_grids, [=, &E, &S, &O, &Alpha, &X, &R, &V](int i)
	{
		X[i] = X_isco + i * delta_X;												// Determine grids in X space
		R[i] = X[i] * X[i];															// Determine grids in R space
		E[i] = E_0 * exp(-1 * (pow((R[i] - Mu), 2.)) / (2. * pow(Sigma, 2.)));		// Calculate initial gaussian mass distribution
		if (E[i] <= 1e-100) E[i] = 1e-6;
		S[i] = X[i] * E[i];															// Calculate initial S values

		O[i] = thompson / m_p;													// Calculate initial opacity values
		Alpha[i] = alpha_hot;													// Initial Alpha parameter values (alpha_hot)
		
		//************************************************************************
		// Calculate viscosity ***************************************************
		V[i] = VISC_C(Alpha[i], O[i]) * pow(X[i], (4. / 3.)) * pow(S[i], (2. / 3.));
		//************************************************************************
		//************************************************************************
	});


	// Inner and outer boundary condition *******************************************************************************************
	//*******************************************************************************************************************************
	E[0]= 0;
	S[0] = 0;
	//E[N_grids - 1] = 0;
	//S[N_grids - 1] = 0;
	M_dot[N_grids - 1] = M_dot_N_grids;
	M_dot[N_grids - 2] = M_dot_N_grids;
	//*******************************************************************************************************************************
	//*******************************************************************************************************************************
	
	// Calculate initial values in a self-consistent way ****************************************************************************
	//*******************************************************************************************************************************
	for (int j = 0; j < 100; j++)
	{
		parallel_for(0, N_grids, [=, &E, &V, &Alpha, &O, &S, &H, &T_c, &T_eff, &V_new, &S_new](int l)
		{
			T_eff[l] = EffectiveTemperature_dubus2014(E[l], R[l], V[l]);
			T_c[l] = CentralTemperature_dubus2014(OpticalThickness(E[l], O[l]), T_eff[l],0);

			if (s_type == full || s_type == thompson_opacity)
			{
				if (T_c[l] > T_c_max(T_irr[l], R[l]) && Alpha[l] == alpha_cold)
					Alpha[l] = alpha_hot;
				if (T_c[l] < T_c_min(T_irr[l], R[l]) && Alpha[l] == alpha_hot)
					Alpha[l] = alpha_cold;

				H[l] = sqrt((T_c[l] * k * pow(R[l], 3)) / (mu_p(Alpha[l]) * m_p * G * M_compact));
				V[l] = Viscosity(T_c[l], Alpha[l], H[l]);
			}
			else if (s_type == analytical)
			{
				H[l] = sqrt((T_c[l] * k * pow(R[l], 3)) / (mu_p(Alpha[l]) * m_p * G * M_compact));
				//************************************************************************
				// Calculate viscosity ***************************************************
				V[l] = VISC_C(Alpha[l], O[l]) * pow(X[l], (4. / 3.)) * pow(S[l], (2. / 3.));
				//************************************************************************
				//************************************************************************
			}

			V_new[l] = V[l];
			S_new[l] = S[l];

			if (s_type == full)
			{
				// Find Opacity value*****************************************************
				//************************************************************************
				int a = 0, b = 0;
				// 19 R values in the table
				for (int m = 0; m < 19; m++)
				{
					if (log10((E[l] / (2 * H[l])) / pow(T_c[l] * 1e-6, 3)) >= logR[18])
					{
						a = 18;
						break;
					}
					if (log10((E[l] / (2 * H[l])) / pow(T_c[l] * 1e-6, 3)) < logR[m])
					{
						a = m;
						break;
					}
				}
				// 88 T values in the table
				for (int n = 0; n < 88; n++)
				{
					if (log10(T_c[l]) >= logT[87])
					{
						b = 87;
						break;
					}
					if (log10(T_c[l]) < logT[n])
					{
						b = n;
						break;
					}
				}

				O[l] = pow(10, opacity[a][b]);
				//************************************************************************
				//************************************************************************
			}
		});
	}
	for (int i = 0; i < N_grids - 2; i++)
	{
		M_dot[i] = 3. * PI * (V[i + 1] * S[i + 1] - V[i] * S[i]) / delta_X;
	}
	//*******************************************************************************************************************************
	//*******************************************************************************************************************************

	J_total = 0;
	M_total = 0;
	for (int i = 0; i < N_grids; i++)
	{
		J_total += 4 * PI * sqrt(G * M_compact) * pow(X[i], 3) * S[i] * delta_X;
		M_total += 4 * PI * S[i] * X[i] * X[i] * delta_X;
	}

	//*************************************************************************************************************
	// Save initial conditions ************************************************************************************
	vector<int> Shadows(N_grids, 0);
	GetShadows(Shadows, R, H, 0, 0);
	WriteGraphData(R, Shadows, 0, N_grids, "ShadowsvsR.txt", false);
	WriteGraphData(R, Alpha, 0, N_grids, "AvsR.txt", false);
	WriteGraphData(R, E, 0, N_grids, "EvsR.txt", false);
	WriteGraphData(R, V, 0, N_grids, "VvsR.txt", false);
	WriteGraphData(R, M_dot, 0, N_grids, "MdotvsR.txt", false);
	WriteGraphData(R, T_eff, 0, N_grids, "TeffvsR.txt", false);
	WriteGraphData(R, T_irr, 0, N_grids, "TirrvsR.txt", false);
	WriteGraphData(R, T_c, 0, N_grids, "TcvsR.txt", false);
	WriteGraphData(R, H, 0, N_grids, "HvsR.txt", false);
	WriteGraphData(R, O, 0, N_grids, "OvsR.txt", false);
	//*************************************************************************************************************
	//*************************************************************************************************************

	if(ProduceAnalytical)
		ProduceAnalyticalSolutions(M_total, J_total);
	auto end = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	cout << "Initial conditions created and analytical solutions produced in " <<	elapsed.count() << " ms.\n";
	cout << "Total angular momentum is " << J_total << " g cm2 s-1.\n";
	cout << "Total mass is " << M_total << " g.\n";
	cout << "Specific angular momentum is " << J_total / M_total << " cm2 s-1.\n";
	//********************************************************************
	//********************************************************************

	cout << "Please enter the time step in seconds.\n";
	cin >> delta_T;
	


	//*************************************************************************************************************
	// File operations ********************************************************************************************
	ofstream file;
	file.open("lightcurve.txt", ios::out);
	if (L_instant > 0)
		file << 0 << "\t" << L_instant << "\n";						// Write luminosity to file
	file.close();

	ofstream file_bolo;
	file_bolo.open("lightcurve_bolo.txt", ios::out);
	if (L_BB > 1e20)
		file_bolo << 0 << "\t" << L_BB << "\n";						// Write luminosity to file
	file_bolo.close();

	ofstream file_HEXTE;
	file_HEXTE.open("lightcurve_HEXTE.txt", ios::out);
	file_HEXTE << 0 << "\t" << L_HEXTE << "\n";
	file_HEXTE.close();

	ofstream file_Optical;
	file_Optical.open("lightcurve_Optical.txt", ios::out);
	file_Optical << 0 << "\t" << L_Optical << "\n";
	file_Optical.close();

	ofstream file_Rhot;
	file_Rhot.open("Rhot_T.txt", ios::out);
	file_Rhot << 0  << "\t" << R[FindHotIndex(Alpha)] << "\n";
	file_Rhot.close();
	//*************************************************************************************************************
	//*************************************************************************************************************
	int s = 1;
	start = chrono::high_resolution_clock::now(); // Start high resolution clock for time measurement
	int time_index = 0;			// time index
	double T = 0;
	double T_corona = T_max;
	while (T < T_max) // iterate over time
	{
		// Determine outer boundary condition***************************************************************************************************************
		//*************************************************************************************************************************************************
		S[N_grids - 1] = pow(((M_dot[N_grids - 2] * delta_X / (3. * PI) + S[N_grids - 2] * V[N_grids - 2])
			/ (VISC_C(Alpha[N_grids - 1], O[N_grids - 1]) * pow(X[N_grids - 1], (4. / 3.)))),
			(3. / 5.));
		V[N_grids - 1] = VISC_C(Alpha[N_grids - 1], O[N_grids - 1]) * pow(X[N_grids - 1], (4. / 3.)) 
			* pow(S[N_grids - 1], (2. / 3.));
		//*************************************************************************************************************************************************
		//*************************************************************************************************************************************************

		for (int j = 0; j < 2; j++)
		{
			//*************************************************************************************************************************************************
			// Generate Tridiagonal matrix ********************************************************************************************************************
			parallel_for(1, N_grids - 1, [=, &V, &S, &Coefficient_Matrix, &Coefficient_A, &Coefficient_B, &Coefficient_C, &Coefficient_D](int i)
			{
				double k = (3. / 8.) * delta_T / pow(delta_X * X[i], 2);
				Coefficient_C[i] = -1 * k * V_new[i + 1];
				Coefficient_B[i] = 1 + 2 * k * V_new[i];
				Coefficient_A[i] = -1 * k * V_new[i - 1];
				if (i == N_grids - 2)
					Coefficient_D[i] = (1 - 2 * k*V[i]) * S[i] + k * V[i - 1] * S[i - 1] + k * V[i + 1] * S[i + 1] + k * V_new[i + 1] * S_new[i + 1];
				else
					Coefficient_D[i] = (1 - 2 * k*V[i]) * S[i] + k * V[i - 1] * S[i - 1] + k * V[i + 1] * S[i + 1];
				/*if (Coefficient_B[i] <= abs(Coefficient_A[i]) + abs(Coefficient_C[i]) && !Diverging)
				{
					cout << "Simulation is diverging... Decreasing time step.\n";
					delta_T = delta_T * 0.99;
					cout << "New time step = " << delta_T << " s\n\n";
					Diverging = true;
				}*/
			});
			/*if (Diverging)
			{
				Diverging = false;
				break;
			}*/
			//*************************************************************************************************************************************************
			//*************************************************************************************************************************************************

			//*************************************************************************************************************************************************
			// Decomposition **********************************************************************************************************************************
			for (int i = 2; i < N_grids - 1; i++)
			{
				Coefficient_A[i] = Coefficient_A[i] / Coefficient_B[i - 1];
				Coefficient_B[i] = Coefficient_B[i] - Coefficient_A[i] * Coefficient_C[i - 1];
			}
			//*************************************************************************************************************************************************
			//*************************************************************************************************************************************************

			//*************************************************************************************************************************************************
			// Forward Substitution ***************************************************************************************************************************
			for (int i = 2; i < N_grids - 1; i++)
			{
				Coefficient_D[i] = Coefficient_D[i] - Coefficient_A[i] * Coefficient_D[i - 1];
			}
			//*************************************************************************************************************************************************
			//*************************************************************************************************************************************************

			//*************************************************************************************************************************************************
			// Backward Substitution ***************************************************************************************************************************
			S_new[N_grids - 2] = Coefficient_D[N_grids - 2] / Coefficient_B[N_grids - 2];
			for (int i = N_grids - 3; i > 0; i--)
			{
				S_new[i] = (Coefficient_D[i] - Coefficient_C[i] * S_new[i + 1]) / Coefficient_B[i];
			}
			//*************************************************************************************************************************************************
			//*************************************************************************************************************************************************

			// Calculate local values in a self-consistent way ****************************************************************************
			//*******************************************************************************************************************************
			for (int m = 0; m < 4; m++)
			{
				if (MaximumLuminosityReached && EnableIrradiation)
				{
					if(!CoronaFormed)
						IrradiationTemperature(L_instant, 0, R, H, T_irr, true);					// Start irradiation
					else
						IrradiationTemperature(L_instant, 500*R_g, R, H, T_irr, true);					// Start irradiation
				}
				if (MaximumLuminosityReached && EnableIrradiation && !EnableShadows)
				{
					IrradiationTemperature_CentralPointSource(L_instant, R, H, T_irr, EnableShadows);					// Start irradiation
				}
				parallel_for(0, N_grids, [=, &O, &H, &T_c, &T_eff, &V, &Alpha, &E, &S, &V_new](int l)
				{
					E[l] = S_new[l] / X[l];   
					T_eff[l] = EffectiveTemperature_dubus2014(E[l], R[l], V_new[l]);
					T_c[l] = CentralTemperature_dubus2014(OpticalThickness(E[l], O[l]), T_eff[l], T_irr[l]);

					if (s_type != analytical)
					{
						if (T_c[l] > T_c_max(T_irr[l], R[l]) && Alpha[l] == alpha_cold)
							Alpha[l] = alpha_hot;
						if (T_c[l] < T_c_min(T_irr[l], R[l]) && Alpha[l] == alpha_hot)
							Alpha[l] = alpha_cold;

						H[l] = sqrt((T_c[l] * k * pow(R[l], 3)) / (mu_p(Alpha[l]) * m_p * G * M_compact));
						V_new[l] = Viscosity(T_c[l], Alpha[l], H[l]);
					}
					else
					{
						H[l] = sqrt((T_c[l] * k * pow(R[l], 3)) / (mu_p(Alpha[l]) * m_p * G * M_compact));
						//************************************************************************
						// Calculate viscosity ***************************************************
						V_new[l] = VISC_C(Alpha[l], O[l]) * pow(X[l], (4. / 3.)) * pow(S_new[l], (2. / 3.));
						//************************************************************************
						//************************************************************************
					}
					if (s_type == full)
					{
						// Find Opacity value*****************************************************
						//************************************************************************
						int a = 0, b = 0;
						// 19 R values in the table
						for (int m = 0; m < 19; m++)
						{
							if (log10((E[l] / (2 * H[l])) / pow(T_c[l] * 1e-6, 3)) >= logR[18])
							{
								a = 18;
								break;
							}
							if (log10((E[l] / (2 * H[l])) / pow(T_c[l] * 1e-6, 3)) < logR[m])
							{
								a = m;
								break;
							}
						}
						// 88 T values in the table
						for (int n = 0; n < 88; n++)
						{
							if (log10(T_c[l]) >= logT[87])
							{
								b = 87;
								break;
							}
							if (log10(T_c[l]) < logT[n])
							{
								b = n;
								break;
							}
						}

						O[l] = pow(10, opacity[a][b]);
						//************************************************************************
						//************************************************************************
					}
				});
			}
			//*************************************************************************************************************************************************
			//*************************************************************************************************************************************************
		}

		parallel_for(0, N_grids, [=, &V, &S, &E](int i)
		{
			S[i] = S_new[i];
			V[i] = V_new[i];
			E[i] = S[i] / X[i];
		});

		for (int i = 0; i < N_grids - 2; i++)
		{
			M_dot[i] = 3. * PI * (V[i + 1] * S[i + 1] - V[i] * S[i]) / delta_X;
		}
		//*******************************************************************************************************************************
		//*******************************************************************************************************************************

		L_instant = 0.1 * (M_dot[0] * G * M_compact) / (2 * R_isco);		// Luminosity in ergs/s

		T += delta_T;

		// Check if corona has been formed***************************************************************************************************************
		//************************************************************************************************************************************************************
		if (EnableCoronaFormation)
		{
			if (MaximumLuminosityReached && L_instant < 0.003 * L_edd && !CoronaFormed && T < T_corona)
			{
				CoronaFormed = true;
				T_corona = T;
				cout << "Corona formed at time T = " << T / day << " days.\n" << elapsed.count() << " ms have elapsed.\n\n";
			}
			else if (CoronaFormed && T > T_corona + 40 * day && EnableCoronaVanishing)
			{
				CoronaFormed = false;
				cout << "Corona has vanished at time T = " << T / day << " days.\n" << elapsed.count() << " ms have elapsed.\n\n";
			}
		}
		//************************************************************************************************************************************************************
		//************************************************************************************************************************************************************

		if (T >= T_max / N_samples * s)
		{
			s++;

			// Check if maximum luminosity has been reached***************************************************************************************************************
			//************************************************************************************************************************************************************
			if (L_instant < L_previous && !MaximumLuminosityReached && L_instant > 0.01 * L_edd)
			{
				cout << std::scientific;
				cout << "Maximum luminosity reached -> L = " << L_instant << " erg/s at time T = " << T / day << " days.\n" << elapsed.count() << " ms have elapsed.\n\n";
				MaximumLuminosityReached = true;
			}
			else if (!MaximumLuminosityReached)
			{
				L_previous = L_instant;
			}
			//************************************************************************************************************************************************************
			//************************************************************************************************************************************************************

			//**************************************************************************************************************************
			// Lightcurves *************************************************************************************************************
			file.open("lightcurve.txt", ios::app);
			file << T / day << "\t" << L_instant << "\n";										// Write luminosity to file
			file.close();

			L_BB = 0.1 * BB_Luminosity(T_eff, X);
			file_bolo.open("lightcurve_bolo.txt", ios::app);
			file_bolo << T / day << "\t" << L_BB << "\n";										// Write luminosity to file
			file_bolo.close();

			L_HEXTE = GetLuminosity(T_eff, X, 3000, 60000, 100);
			file_bolo.open("lightcurve_HEXTE.txt", ios::app);
			file_bolo << T / day << "\t" << L_HEXTE << "\n";									// Write luminosity to file
			file_bolo.close();

			L_Optical = GetLuminosity(T_eff, X, 1, 4, 0.001);
			file_Optical.open("lightcurve_Optical.txt", ios::app);
			file_Optical << T / day << "\t" << L_Optical << "\n";									// Write luminosity to file
			file_Optical.close();

			file_Rhot.open("Rhot_T.txt", ios::app);
			file_Rhot << T / day << "\t" << R[FindHotIndex(Alpha)] << "\n";						// Write luminosity to file
			file_Rhot.close();
			//**************************************************************************************************************************
			//**************************************************************************************************************************

			//**************************************************************************************************************************
			// Take samples ************************************************************************************************************
			GetShadows(Shadows, R, H, 0, 0);
			WriteGraphData(R, Shadows, T, N_grids, "ShadowsvsR.txt", true);
			WriteGraphData(R, Alpha, T, N_grids, "AvsR.txt", true);
			WriteGraphData(R, E, T, N_grids, "EvsR.txt", true);
			WriteGraphData(R, V, T, N_grids, "VvsR.txt", true);
			WriteGraphData(R, M_dot, T, N_grids, "MdotvsR.txt", true);
			WriteGraphData(R, T_eff, T, N_grids, "TeffvsR.txt", true);
			WriteGraphData(R, T_irr, T, N_grids, "TirrvsR.txt", true);
			WriteGraphData(R, T_c, T, N_grids, "TcvsR.txt", true);
			WriteGraphData(R, H, T, N_grids, "HvsR.txt", true);
			WriteGraphData(R, O, T, N_grids, "OvsR.txt", true);
			//**************************************************************************************************************************
			//**************************************************************************************************************************

			end = std::chrono::high_resolution_clock::now();
			elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			cout << std::scientific;
			J_total = 0;
			M_total = 0;
			for (int i = 0; i < N_grids; i++)
			{
				J_total += 4 * PI * sqrt(G * M_compact) * pow(X[i], 3) * S[i] * delta_X;
				M_total += 4 * PI * S[i] * X[i] * X[i] * delta_X;
			}
			cout << "Current time is           " << T / day << " days.\n";
			cout << "Luminosity is             " << L_instant << " erg/s (i.e. " << L_instant/L_edd * 100 << " \% of Eddington luminosity).\n";
			cout << "Total angular momentum is " << J_total << " g cm2 s-1.\n";
			cout << "Total mass is             " << M_total << " g.\n";
			cout << (double)T / T_max * 100 << fixed << " percent completed! " << elapsed.count() << " ms have elapsed.\n\n";
		}
	}
	cout << "All done!";
	if (argc == 1)
	{
		cin.get();
		cin.get();
	}
	return 0;
}

double NormalizationGaussian(double Mu, double Sigma, double R_isco, double R_outer, double M_disk)
{
	const double PI = 3.14159265358979323846;
	double IntegralResult = 0.5 * Sigma * (2 * (exp(-0.5 * pow((R_isco - Mu), 2) / pow(Sigma, 2)) - exp(-0.5 *pow((R_outer - Mu), 2) / pow(Sigma, 2))) * Sigma
		- Mu * sqrt(2 * PI) * erf((R_isco - Mu) / (sqrt(2) * Sigma)) + Mu * sqrt(2 * PI) * erf((R_outer - Mu) / (sqrt(2) * Sigma)));
	double E_0 = M_disk / (2 * PI * IntegralResult);
	return E_0;
}

void WriteGraphData(double* X, double* Y, int length, string filename, bool append)
{
	FILE* file;
	if (!append)
		file = fopen(filename.c_str(), "w");
	else
		file = fopen(filename.c_str(), "a");
	parallel_for(0, length, [=, &file](int i)
	{
		fprintf(file, "%lf\t%lf\n", X[i], Y[i]);
	});
	fprintf(file, "\n");
	fclose(file);
}
void WriteGraphData(vector<double> X, vector<int> Y, double T, int length, string filename, bool append)
{
	FILE* file;
	if (!append)
		file = fopen(filename.c_str(), "w");
	else
		file = fopen(filename.c_str(), "a");
	for (int i = 0; i < length; i++)
	{
		fprintf(file, "%lf\t%lf\t%d\n", T / day, X[i], Y[i]);
	}
	fprintf(file, "\n");
	fclose(file);
}
void WriteGraphData(vector<double> X, vector<double> Y, double T, int length, string filename, bool append)
{
	FILE* file;
	if (!append)
		file = fopen(filename.c_str(), "w");
	else
		file = fopen(filename.c_str(), "a");
	for(int i=0; i < length; i++)
	{
		fprintf(file, "%lf\t%lf\t%lf\n", T/day, X[i], Y[i]);
	}
	fprintf(file, "\n");
	fclose(file);
}

double OpticalThickness(double SurfaceDensity, double opacity)
{
	return SurfaceDensity * opacity;
}

double BB_Luminosity(vector<double> T_eff, vector<double> X)
{
	double L = 0;
	for(int i = 0; i < N_grids; i++)
	{
		L += pow(X[i], 3) * pow(T_eff[i], 4) * a * 4 * PI * delta_X;
	}
	return L;
}

double BB_Luminosity(double* T_eff, double* X, int truncation_index)
{
	double L = 0;
	for (int i = truncation_index; i < N_grids; i++)
	{
		L += pow(X[i], 3) * pow(T_eff[i], 4) * a * 4 * PI * delta_X;
	}
	return L;
}

void ExtractSpectrum(double* T, double* X, double delta_X, int N_grids, double minEnergyEV, double maxEnergyEV, double resolutionEV, bool append)
{
	int numberofchannels = (maxEnergyEV - minEnergyEV) / resolutionEV;
	double* I_bb = new double[numberofchannels];
	double* eV = new double[numberofchannels];
	parallel_for(0, numberofchannels, [=](int i)
	{
		I_bb[i] = 0;
		eV[i] = minEnergyEV + resolutionEV * i;
	});
	parallel_for(0, N_grids - 2, [=](int i)
	{
		parallel_for(0, numberofchannels, [=](int j)
		{
			if(T[i + 1] != 0)
				I_bb[j] += (2 * h * pow(eVtoHz(eV[j]), 3) / pow(c, 2)) / (exp((h * eVtoHz(eV[j])) /
					(k * T[i + 1])) - 1)
					* 4 * PI * pow(X[i], 3) * delta_X; // infinitesimal area
		});
	});
	WriteGraphData(eV, I_bb, numberofchannels, "powerspectrum.txt", append);
}

double GetLuminosity(vector<double> T, vector<double> X, double minEnergyEV, double maxEnergyEV, double resolutionEV)
{
	int numberofchannels = (maxEnergyEV - minEnergyEV) / resolutionEV;
	double* I_bb = new double[numberofchannels];
	double* eV = new double[numberofchannels];
	parallel_for(0, numberofchannels, [=](int i)
	{
		I_bb[i] = 0;
		eV[i] = minEnergyEV + resolutionEV * i;
	});
	for (int i = 0; i < N_grids; i++)
	{
		for (int j = 0; j < numberofchannels; j++)
		{
			if (T[i] != 0)
				I_bb[j] += (2 * h * pow(eVtoHz(eV[j]), 3) / pow(c, 2)) / (exp((h * eVtoHz(eV[j])) /
				(k * T[i])) - 1)
				* 4 * PI * pow(X[i], 3) * delta_X; // infinitesimal area
		}
	}
	double L = 0;
	for (int i = 0; i < numberofchannels; i++)
	{
		L += I_bb[i];
	}
	return L;
}

void IrradiationTemperature_CentralPointSource(double LUMINOSITY, vector<double> R, vector<double> H, vector<double> &T_irr, bool ShadowEnabled)
{
	vector<int> Shadows(N_grids, 0);
	if (ShadowEnabled)
		GetShadows(Shadows, R, H, 0, 0);									// Get shadows 
	//else
		//LUMINOSITY = 10 * LUMINOSITY;										// Increase luminosity if corona has been formed
	
	parallel_for(0, N_grids - 1, [=, &T_irr](int i)
	{
		if (Shadows[i] == 0)
		{
			double d_2 = R[i] * R[i] + H[i] * H[i];						// Distance from central point source
			double theta = atan(H[i + 1] / R[i + 1]);					// elevation of disk element
			double tan_phi = (H[i + 1] - H[i]) / (R[i + 1] - R[i]);		
			double phi = atan(tan_phi);									// slope angle of disk element
			double Flux_disk = LUMINOSITY / (d_2 * 4 * PI) * sin(abs(theta - phi));
			T_irr[i] = pow((Flux_disk / a), 0.25);
		}
		else
			T_irr[i] = 0;
	});

}
void IrradiationTemperature(double LUMINOSITY, double R_corona, double Theta_corona, vector<double> R, vector<double> H, vector<double> &T_irr, bool ShadowEnabled)
{
	Theta_corona = Theta_corona / 180 * PI; // from degrees to radian
	vector<int> Shadows(N_grids, 0);
	if (ShadowEnabled)
		GetShadows(Shadows, R, H, R_corona, Theta_corona);									// Get shadows 
																			//else
	if(R_corona > 5*R_g)
		LUMINOSITY = 100 * LUMINOSITY;										// Increase luminosity if corona has been formed

	parallel_for(0, N_grids - 1, [=, &T_irr](int i)
	{
		if (Shadows[i] == 0)
		{
			double d_2 = (R[i] - R_corona*cos(Theta_corona)) * (R[i] - R_corona*cos(Theta_corona)) + (H[i] - R_corona*sin(Theta_corona)) * (H[i] - R_corona*sin(Theta_corona));		// Distance from point source
			double theta = atan((H[i + 1] - R_corona*sin(Theta_corona)) / (R[i + 1] - R_corona*cos(Theta_corona)));					// elevation of disk element
			double tan_phi = (H[i + 1] - H[i]) / (R[i + 1] - R[i]);
			double phi = atan(tan_phi);									// slope angle of disk element
			
			double lambda = Theta_corona - theta;
			double psi = theta - phi + PI / 2;
			double Flux_disk = abs(LUMINOSITY / (d_2 * 4 * PI) * cos(lambda) * cos(psi));
			if (Flux_disk > 0)
				T_irr[i] = pow((Flux_disk / a), 0.25);
			else
				T_irr[i] = 0;
		}
		else
			T_irr[i] = 0;
	});

}

void IrradiationTemperature(double LUMINOSITY, double R_corona, vector<double> R, vector<double> H, vector<double> &T_irr, bool ShadowEnabled)
{
	vector<int> Shadows(N_grids, 0);
	if (ShadowEnabled)
		GetShadows(Shadows, R, H, R_corona);									// Get shadows 
																							//else
	if (R_corona > 5 * R_g)
		LUMINOSITY = 100 * LUMINOSITY;										// Increase luminosity if corona has been formed
	parallel_for(0, N_grids - 1, [=, &T_irr](int i)
	{
		if (Shadows[i] == 0)
		{
			double d_2 = R[i] * R[i] + (H[i] - R_corona) * (H[i] - R_corona);		// Distance from point source
			
			double tan_phi = (H[i + 1] - H[i]) / (R[i + 1] - R[i]);
			double phi = atan(tan_phi);									// slope angle of disk element
			
			double beta = atan((H[i] - R_corona) / R[i]);
			
			double lambda = phi - beta;
			
			double Flux_disk = abs(LUMINOSITY / (d_2 * 4 * PI) * sin(lambda));
			
			if (Flux_disk > 0)
				T_irr[i] = pow((Flux_disk / a), 0.25);
			else
				T_irr[i] = 0;
		}
		else
			T_irr[i] = 0;
	});
}

void GetShadows(vector<int> &Shadows, vector<double> R, vector<double> H, double R_corona)
{
	parallel_for(0, N_grids - 1, [=, &Shadows](int i)
	{
		int n = 0;
		for (int j = 0; j < i + 1; j++)
		{
			if (atan((H[i + 1] - R_corona) / R[i + 1]) <
				atan((H[j] - R_corona) / R[j]))
			{
				n++;
			}
		}
		if (n > 3)
		{
			Shadows[i + 1] = 1;
		}
		else Shadows[i + 1] = 0;
	});
	Shadows[0] = 0;
}

void GetShadows(vector<int> &Shadows, vector<double> R, vector<double> H, double R_corona, double Theta_corona)
{
	Shadows[0] = 0;
	parallel_for(0, N_grids - 1, [=, &Shadows](int i)
	{
		int n = 0;
		for (int j = 0; j < i + 1; j++)
		{
			if (atan((H[i + 1] - R_corona * sin(Theta_corona)) / abs(R[i + 1] - R_corona * cos(Theta_corona))) <
				atan((H[j] - R_corona * sin(Theta_corona)) / abs(R[j] - R_corona * cos(Theta_corona))))
			{
				n++;
			}	
		}
		if (n > 3)
		{
			Shadows[i + 1] = 1;
		}
		else Shadows[i + 1] = 0;
	});
}

double eVtoHz(double eV)
{
	return 2.417990504024e+14 * eV; // not sure...
}

double mu_p(double alpha)
{
	if (alpha == alpha_hot)
		return mu_p_hot;
	else return mu_p_cold;
}

int FindHotIndex(vector<double> vAlpha)
{
	int i;
	for (i = 0; i < N_grids - 1; i++)
	{
		if (vAlpha[i] == alpha_cold && vAlpha[i + 1] == alpha_cold)
		{
			return i;
		}
	}
	return N_grids - 1;
}

double VISC_C(double alpha, double opacity)
{
	double VIS_C = pow(
		(27. / 32.) * (pow(alpha, 4) * pow(k, 4) * opacity)
		/ (pow(mu_p(alpha), 4) * pow(m_p, 4) * a * G * M_compact)
		, (1. / 3.));
	return VIS_C;
}

double average(double numbers[], int size) {
	double sum = 0;
	parallel_for(0, size, [=, &sum](int i)
	{
		if(numbers[i] < 100)
			sum += numbers[i];
	});
	return sum / (double)size;
}

double T_critical(double T_irr, double R)
{
	if (T_c_max(T_irr, R) == std::numeric_limits<double>::infinity())
		return 0;
	return 0.5 * (T_c_max(T_irr, R) + T_c_min(T_irr, R));
}
double T_c_max(double T_irr, double R)
{
	return 10700 * pow(alpha_cold, -0.1) * pow(R / 1e10, -0.05 * irr_effect(T_irr));
}
double T_c_min(double T_irr, double R)
{
	return (20900 - 11300 * irr_effect(T_irr)) * pow(alpha_hot, -0.22)* pow(M_compact / M_solar, -0.01) * pow(R / 1e10, 0.05 - 0.12 * irr_effect(T_irr));
}
double irr_effect(double T_irr)
{
	return pow(T_irr / 1e4, 2);
}
double E_max(double T_irr, double R)
{
	return (10.8 - 10.3 * irr_effect(T_irr)) * pow(alpha_cold, -0.84) * pow(M_compact / M_solar, -0.37 + 0.1 * irr_effect(T_irr))
		* pow(R / 1e10, 1.11 - 0.27 * irr_effect(T_irr));
}
double E_min(double T_irr, double R)
{
	return (8.3 - 7.1 * irr_effect(T_irr)) * pow(alpha_hot, -0.77) * pow(M_compact / M_solar, -0.37)
		* pow(R / 1e10, 1.12 - 0.23 * irr_effect(T_irr));
}

double EffectiveTemperature_dubus2014(double E, double R, double V)
{
	return pow((9. / 8.) * V * E * G * M_compact / (a * pow(R, 3)), 0.25);
}

double CentralTemperature_dubus2014(double tau, double T_eff, double T_irr)
{
	return pow(((3. / 8.) * tau * pow(T_eff, 4) + pow(T_irr, 4)), 0.25);
}

void RadiationPressure(double* P_rad, double* T_c)
{
	parallel_for(0, N_grids, [=](int i)
	{
		P_rad[i] = 4. / 3. * a / c /* rad_const*/ * pow(T_c[i], 4);
	});
}

void GasPressure(double* P_gas, double* E, double* H, double* T_c, double* Alpha)
{
	parallel_for(0, N_grids, [=](int i)
	{
		P_gas[i] = ((E[i] / H[i]) * k * T_c[i]) / (mu_p(Alpha[i]) * m_p);
	});
}

void ScaleHeight_FrankKingRaine(double * H, double * E, double * T_c, double* M_dot, double* R, double* Alpha)
{
	double* P_rad = new double[N_grids];
	double* P_gas = new double[N_grids];
	RadiationPressure(P_rad, T_c);
	GasPressure(P_gas, E, H, T_c, Alpha);

	for (int j = 0; j < 10; j++)
	{
		parallel_for(0, N_grids, [=](int i)
		{
			if (P_gas[i]*10 < P_rad[i])
			{
				H[i] = ((3 * R_isco) / (4 * 0.1)) * (M_dot[i] / M_dot_edd) * (1 - pow(R_isco / R[i], 0.5));
			}
			else
			{
				H[i] = sqrt((T_c[i] * k * pow(R[i], 3)) / (mu_p(Alpha[i]) * m_p * G * M_compact));
			}
		});
		GasPressure(P_gas, E, H, T_c, Alpha);
	}
}

double TimeDependentCoronaRadius(double T_current, double T_corona, double T_rise, double R_initial, double R_max, bool Formed)
{
	if (Formed)
	{
		if (T_current < T_corona + T_rise)
		{
			double c_1 = (R_initial - R_max) / pow(T_rise, 2);
			double c_2 = -2 * c_1 * (T_corona + T_rise);
			double c_3 = R_max + pow(T_corona + T_rise, 2) * c_1;
			return c_1 * pow(T_current, 2) + c_2 * T_current + c_3;
		}
		else
			return R_max;
	}
	else return R_isco;
}

void printmatrix(vector<vector<double>> matrix, int rows, int columns)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			cout << setprecision(1);
			cout << std::setw(8);
			cout << matrix[i][j] << " ";
		}
		cout << "\n";
	}
}

void printmatrix(double* matrix, int rows)
{
	for (int i = 0; i < rows; i++)
	{
			cout << setprecision(1);
			cout << std::setw(8);
			cout << matrix[i];
		cout << "\n";
	}
}

double SoundSpeed(double T_c, double alpha)
{
	return sqrt(k * T_c / (mu_p(alpha) * m_p));
}

double Viscosity(double T_c, double alpha, double H)
{
	return alpha * H * SoundSpeed(T_c, alpha);
}

void ProduceAnalyticalSolutions(double M_disk, double J_disk)
{
	double M_0 = M_disk;
	double J_0 = J_disk;
	for (int m = 0; m < 100; m++)
	{
		double alpha = alpha_cold + m * 0.01;

		double p = 1, q = 2. / 3.;
		double a_ = 1 + 1 / (5 * q - 2 * p + 4);
		double k = pow(q / ((4 * q - 2 * p + 4)* (5 * q - 2 * p + 4)), 1 / q);
		double beta = tgamma((q + 1) / q) * tgamma((5 * q - 2 * p + 5) / (4 * q - 2 * p + 4)) /
			tgamma((5 * q - 2 * p + 5) / (4 * q - 2 * p + 4) + (q + 1) / q);

		double gamma1 = k * (q + 1) / (4 * q - 2 * p + 4) * beta;
		double gamma2 = k * q / (4 * q - 2 * p + 4);

		double r_0 = pow(J_0 / M_0, 2) / (G * M_compact) * pow(gamma2 / gamma1, 2);
		double E_0 = M_0 / (4 * PI * r_0 * r_0 * gamma2);
		double v_0 = VISC_C(alpha, thompson / m_p) * pow(r_0, p) * pow(E_0, q);
		double t_0 = 4 * r_0 * r_0 / (3 * v_0);


		double* time_array = new double[N_samples];
		vector<double> R(N_grids, 0);											// Surface mass density
		vector<double> E(N_grids, 0);											// Surface mass density
		vector<double> T_c(N_grids, 0);											// Surface mass density
		vector<double> T_eff(N_grids, 0);											// Surface mass density
		vector<double> V(N_grids, 0);											// Surface mass density
		vector<double> Tc_max(N_grids, 0);											// Surface mass density
		vector<double> Tc_min(N_grids, 0);											// Surface mass density
		double time_step = T_max / N_samples;
		double grid_step = X_outer / N_grids;

		cout << "In T = " << T_max / month << " months the disk will extend to " << r_0 * pow(1 + T_max / t_0, 2 / (5 * q - 2 * p + 4)) << " cm.\n";

		parallel_for(0, N_samples, [=](int i)
		{
			time_array[i] = time_step * (i + 1);
		});
		parallel_for(0, N_grids, [=, &R, &E, &V, &T_c, &T_eff, &Tc_max, &Tc_min](int i)
		{
			R[i] = pow(X_isco + i * grid_step, 2);
			E[i] = E_0 * (k * pow(R[i] / r_0, -p / (q + 1)) * pow(1 - pow(R[i] / r_0, 2 - p / (q + 1)), 1 / q));
			//************************************************************************
			// Calculate viscosity ***************************************************
			V[i] = VISC_C(alpha, thompson / m_p) * pow(R[i], p) * pow(E[i], q);
			//************************************************************************
			//************************************************************************
			T_eff[i] = EffectiveTemperature_dubus2014(E[i], R[i], V[i]);
			T_c[i] = CentralTemperature_dubus2014(OpticalThickness(E[i], thompson / m_p), T_eff[i], 0);

			Tc_max[i] = T_c_max(0, R[i]);
			Tc_min[i] = T_c_min(0, R[i]);

			if (isnan(E[i]))
				E[i] = 0;
			if (isnan(V[i]))
				V[i] = 0;
			if (isnan(T_eff[i]))
				T_eff[i] = 0;
			if (isnan(T_c[i]))
				T_c[i] = 0;
			if (isnan(Tc_max[i]))
				Tc_max[i] = 0;
			if (isnan(Tc_min[i]))
				Tc_min[i] = 0;
		});

		ofstream file;
		char file_name[30];
		sprintf(file_name, "lightcurve_analytic_%f.txt", alpha);
		file.open(file_name, ios::out);
		file << 0 / day << "\t" << 0.1 * (G * M_compact) / (2 * R_isco) *
			(a_ - 1) * M_0 / t_0 << "\n";						// Write luminosity to file
		file.close();

		WriteGraphData(R, E, 0, N_grids, "EvsR_analytic.txt", false);
		WriteGraphData(R, V, 0, N_grids, "V_analytic.txt", false);
		WriteGraphData(R, T_c, 0, N_grids, "Tc_analytic.txt", false);
		WriteGraphData(R, T_eff, 0, N_grids, "Teff_analytic.txt", false);
		WriteGraphData(R, Tc_max, 0, N_grids, "Tcmax.txt", false);
		WriteGraphData(R, Tc_min, 0, N_grids, "Tcmin.txt", false);

		for (int i = 0; i < N_samples; i++)
		{
			double R_out = r_0 * pow(1 + time_array[i] / t_0, 2 / (5 * q - 2 * p + 4));

			parallel_for(0, N_grids, [=, &R, &E, &V, &T_c, &T_eff, &Tc_max, &Tc_min](int j)
			{
				E[j] = E_0 * k * pow(1 + time_array[i] / t_0, -5 / (5 * q - 2 * p + 4))
					* pow(R[j] / R_out, -p / (q + 1)) * pow(1 - pow(R[j] / R_out, 2 - p / (q + 1)), 1 / q);
				//************************************************************************
				// Calculate viscosity ***************************************************
				V[j] = VISC_C(alpha, thompson / m_p) * pow(R[j], p) * pow(E[j], q);
				//************************************************************************
				//************************************************************************
				T_eff[j] = EffectiveTemperature_dubus2014(E[j], R[j], V[j]);
				T_c[j] = CentralTemperature_dubus2014(OpticalThickness(E[j], thompson / m_p), T_eff[j], 0);
				Tc_max[j] = T_c_max(0, R[j]);
				Tc_min[j] = T_c_min(0, R[j]);
				if (isnan(E[j]))
					E[j] = 0;
				if (isnan(V[j]))
					V[j] = 0;
				if (isnan(T_eff[j]))
					T_eff[j] = 0;
				if (isnan(T_c[j]))
					T_c[j] = 0;
				if (isnan(Tc_max[j]))
					Tc_max[j] = 0;
				if (isnan(Tc_min[j]))
					Tc_min[j] = 0;
			});

			file.open(file_name, ios::app);
			file << time_array[i] / day << "\t" << 0.1 * (G * M_compact) / (2 * R_isco) *
				(a_ - 1) * M_0 / t_0 * pow(1 + time_array[i] / t_0, -1 * a_) << "\n";		// Write luminosity to file
			file.close();
			WriteGraphData(R, V, time_array[i], N_grids, "V_analytic.txt", true);
			WriteGraphData(R, T_c, time_array[i], N_grids, "Tc_analytic.txt", true);
			WriteGraphData(R, T_eff, time_array[i], N_grids, "Teff_analytic.txt", true);
			WriteGraphData(R, E, time_array[i], N_grids, "EvsR_analytic.txt", true);
			WriteGraphData(R, Tc_max, time_array[i], N_grids, "Tcmax.txt", true);
			WriteGraphData(R, Tc_min, time_array[i], N_grids, "Tcmin.txt", true);
		}
	}

}
