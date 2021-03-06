#ifndef __SHARED_H__
#define __SHARED_H__

#define MAX(A,B) ((A) > (B) ? (A) : (B))
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define ll long long
#define pb push_back
#define mp make_pair
#define pii pair<int,int>
#define pdd pair<double,double>
#define pid pair<int,double>
#define pdi pair<double,int>
#define pld pair<ll,double>
#define pll pair<ll,ll>
#define DEBUG true
#define DEBUG_LOG(A) if(DEBUG) cout << (A) << endl
#define repeat(SIZE) for(uint index = 0; index < (SIZE); index++)
#define times(VAR,SIZE) for(uint VAR = 0; VAR < (SIZE); VAR++)
#define INF 99999999
#define MAX_DOCS 20000
#define MAX_CLUSTERS 100 
#define uint unsigned int
#define ull unsigned long long 

#ifndef __MAIN_FILE__

using namespace std;

enum METHOD { FCM, PCM, PFCM };
enum NORM { EUCLIDIAN, COSINE, JACCARD };
struct arguments {
  METHOD mode;
  NORM norm;
  bool verbose;
  bool random;
  char *input;
  const char *path;
  double a,b,m,n,r,epsilon;
  double min_m, min_n;
  int c, min_clusters, sampling;
};

vector<string> terms;
vector<double> docs[MAX_DOCS];
vector<double> prototypes[MAX_CLUSTERS];
vector<uint>    clusters[MAX_CLUSTERS];
vector<double> memberships[MAX_DOCS];
vector<double> tipicalities[MAX_DOCS];
vector<double> gamas(MAX_CLUSTERS, 0);
vector<double> final_memberships[MAX_DOCS];
vector<double> final_tipicalities[MAX_DOCS];
vector<double> merged[MAX_DOCS];
vector<pii > crisp(MAX_DOCS, mp(0,1));
uint num_terms;
uint num_descriptors = 10;
uint num_docs;
uint num_clusters = 3;
uint max_groups = 3;
double fuzziness = 1.2; 
double epsilon = 0.01;
double a = 1;
double b = 2;
double fuzziness_n = 1.2; 
double fuzziness_m = 1.2; 
struct arguments arguments;

#else

extern std::vector<string> terms;
extern std::vector<double> docs[MAX_DOCS];
extern std::vector<double> prototypes[MAX_CLUSTERS];
extern std::vector<double> tipicalities[MAX_DOCS];
extern std::vector<uint>  clusters[MAX_CLUSTERS];
extern std::vector<double> memberships[MAX_DOCS];
extern std::vector<double> final_memberships[MAX_DOCS];
extern vector<double> merged[MAX_DOCS];
extern vector<double> gamas;
extern std::vector<pii> crisp;
extern uint num_terms;
extern uint num_docs;
extern uint num_clusters;
extern uint max_groups;
extern double fuzziness; 
extern double epsilon;
extern double a;
extern double b;
extern double fuzziness_n = 1.2; 
extern double fuzziness_m = 1.4; 
extern struct arguments arguments;

#endif

static inline map<uint,bool> sampling(uint n, uint m){
  //C++-ish pseudocode
  vector<uint> arr(n);
  map<uint,bool> sample;
  for(uint i = 0; i < n; ++i){
    arr[i] = i;
    sample[i] = ( m == 0 );
  }

  srand (time(NULL));
  for(uint i = 0; i < m; ++i){
    uint tmp = arr[n-i];
    uint pos = rand()%(n-i);
    arr[n-i] = arr[pos];
    arr[pos] = tmp;
    sample[arr[n-i]] = true;
  }
  return sample;
}

static inline void read_data(){

  string line;

  cin >> num_terms >> num_docs;

  repeat(num_terms){
    cin >> line;
    terms.pb(line);
    //DEBUG_LOG(line);
  }

  map<uint,bool> sample = sampling(num_docs, arguments.sampling);

  uint doc = 0;
  printf("Sampling %d from %d\n", arguments.sampling, num_docs);
  times(i, num_docs){
    double frequency; 
    times(j, num_terms){
      cin >> frequency; 
      if(sample[i]){
        docs[doc].pb(frequency);
      }
    }
    if(sample[i]){
      printf("%d,", i);
      doc++;
    }
  }
  printf("\n");
  if(arguments.sampling > 0) num_docs = arguments.sampling;
}


static inline void save_matrix(string fname, vector<double> *matrix, uint size) {
    uint i, j;
    FILE *f;
    if(fname == ""){
      f = stdout;
    }else if ((f = fopen(fname.c_str(), "w")) == NULL) {
        fprintf(stderr, "Cannot create output file.\n");
        exit(1);
    }
    for (i = 0; i < size; i++) {
        for (j = 0; j < matrix[i].size(); j++) {
            fprintf(f, "%lf\t", matrix[i][j]);
        }
        fprintf(f, "\n");
    }
    if(fname != "")
      fclose(f);
}


static inline double euclidian_norm(uint i, uint j, vector<double> *x, vector<double> *y) {
  double sum = 0.0;
  #pragma omp parallel for reduction(+:sum)
  for (uint k = 0; k < num_terms; k++) {
    sum += pow(x[i][k] - y[j][k], 2);
  }
  return sqrt(sum);
}

static inline double inner_product(uint i, uint j, vector<double> *x, vector<double> *y){
  double sum = 0.0;
  #pragma omp parallel for reduction(+:sum)
  for (uint k = 0; k < num_terms; k++) {
    sum += x[i][k] * y[j][k];
  }
  return sum;
}

static inline double cosine_norm(uint i, uint j, vector<double> *x, vector<double> *y){
    double numerator = inner_product(i, j, x, y);
    if(numerator == 0) return 1;
    double inner_x = inner_product(i, i, x, x);
    double inner_y = inner_product(j, j, y, y);
    return 1 - ( numerator / ( sqrt(inner_x) * sqrt(inner_y) ));
}

static inline double jaccard_norm(uint i, uint j, vector<double> *x, vector<double> *y){
    double numerator = inner_product(i, j, x, y);
    if(numerator == 0) return 1;
    double inner_x = inner_product(i, i, x, x);
    double inner_y = inner_product(j, j, y, y);
    return 1 - ( numerator / ( inner_x + inner_y - numerator ));
}

static inline double get_norm(uint i, uint j, vector<double> *x, vector<double> *y){
  if(arguments.norm == EUCLIDIAN)
    return euclidian_norm(i, j, x, y);
  if(arguments.norm == COSINE)
    return cosine_norm(i, j, x, y);
  if(arguments.norm == JACCARD)
    return jaccard_norm(i, j, x, y);
  return -1;
}

static inline double get_new_value(uint i, uint j) {
    uint k;
    double t, p, sum;
    sum = 0.0;
    p = 2 / (fuzziness - 1);
    for (k = 0; k < num_clusters; k++) {
        t = get_norm(i, j, docs, prototypes) / get_norm(i, k, docs, prototypes);
        t = pow(t, p);
        sum += t;
    }
	if(isnan(sum)) return 0;
    return 1.0 / sum;
}

static inline void debug_memberships(){
  times(i, num_docs){
    times(j, num_clusters){
      printf("%.8f\t", memberships[i][j]);
    }
    printf("\n");
  }
}

static inline void generate_memberships(){
  double s,rval;
  uint range = ( RAND_MAX/(num_clusters+1) )+1;
  crisp.clear();
  srand (time(NULL));
  for (uint i = 0; i < num_docs; i++) {
    s = 0.0;
    crisp.pb(mp(0,1));
    memberships[i].clear();
    for (uint j = 0; j < num_clusters; j++) {
      rval = rand() % (range);
      memberships[i].pb(rval);
      s += memberships[i][j];
    }
    for (uint j = 0; j < num_clusters; j++) {
      memberships[i][j] /= s;
    }
  }
}

static inline void init_prototypes(){
  times(i, num_clusters){
    prototypes[i].clear();
    times(j, num_terms){
      prototypes[i].pb(0);
    }
  }
}

static inline double aswc(){
  double fs = 0;
  #pragma omp parallel for
  times(i, num_docs){
    double max1_degree = 0;
    double max2_degree = 0;
    times(j, num_clusters){
      if(j == 0){
        crisp[i] = mp(0,1);
        max1_degree = memberships[i][0];
        max2_degree = memberships[i][1];
      }else if(memberships[i][j] > max1_degree){
        max1_degree = memberships[i][j];
        pii item = crisp[i];
        crisp[i] = mp(j,item.first);
        max2_degree = memberships[i][item.first];
      }else if(memberships[i][j] > max2_degree){
        max2_degree = memberships[i][j];
        crisp[i].second = j;
      }
    }
  }
  double sum_up = 0, sum_down = 0;
  #pragma omp parallel for reduction(+:sum_up) reduction(+:sum_down)
  times(i, num_docs){
    double s;
    uint g = crisp[i].first;
    vector<double> alphas(num_clusters, 0);
    vector<uint> alphas_count(num_clusters, 0);
    //times(j, num_docs){
    //  int grupo = crisp[j].first;
    //  if(grupo != g) continue;
    //  alphas[grupo] += norm_doc2doc(i, j);
    //  alphas_count[grupo]++;
    //}
    //double alpha_g = alphas[g] / alphas_count[g];
    double alpha_g = get_norm(i, g, docs, prototypes);
    double beta = INF;
    times(h, num_clusters){
      if(h != g){
        beta = MIN(beta, get_norm(i, h, docs, prototypes));
      }
    }
    if(beta == INF)
      s = -1;
    else
      s = (beta - alpha_g)/MAX(beta, alpha_g);
    double u1 = memberships[i][crisp[i].first];
    double u2 = memberships[i][crisp[i].second];
    sum_up += s * (u1 - u2);
    sum_down += (u1 - u2);
    //if(arguments.verbose) 
    //  printf("d%d: %lf beta %lf alpha %lf u1 %lf u2 %lf s %lf sumup %lf sumdown\n", i, beta,
    //    alpha_g, u1, u2, s, sum_up, sum_down);
  }
  fs = sum_up / sum_down;
  return fs;
}

static inline void store_final_memberships(){
  #pragma omp parallel for
  times(i, num_docs){
    final_memberships[i].swap(memberships[i]);
  }
}
static inline void store_final_tipicalities(){
  #pragma omp parallel for
  times(i, num_docs){
    final_tipicalities[i].swap(tipicalities[i]);
  }
}

#endif
