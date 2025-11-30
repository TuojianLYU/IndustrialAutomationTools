import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
import regex as re
from jobspy import scrape_jobs
import random
import time

# --- CONFIGURATION ---
# Nordic & EU Industrial Hubs
COUNTRIES = ['Germany', 'Netherlands', 'France', 'Sweden', 'Finland', 'Norway']

# Default IT/OT Convergence Search Terms
DEFAULT_SEARCH_TERMS = [
    "IT OT Convergence Engineer",
    "Edge Computing Engineer",
    "Industrial IoT Developer",
    "PLC Software Engineer",
    "Embedded Systems Cloud",
    "OT Security Engineer",
    "Smart Factory Engineer",
    "Industry 4.0 Engineer"
]

RESULTS_WANTED = 15  # Keep low (10-20) to avoid IP bans during testing
USE_MOCK_DATA_ON_FAIL = True  # If scraping fails, generate dummy data

# IT/OT Convergence Skills Keywords
SKILL_KEYWORDS = {
    "Embedded/OT": ["C", "C++", "Rust", "RTOS", "Microcontrollers", "ARM", "Firmware", "Drivers", "Bare-metal"],
    "PLC/Automation": ["PLC", "SCADA", "HMI", "Siemens", "Beckhoff", "Allen Bradley", "IEC 61131", "TIA Portal", "Codesys"],
    "Cloud/IT": ["AWS", "Azure", "GCP", "Kubernetes", "Docker", "Terraform", "CI/CD", "DevOps", "Microservices"],
    "Edge Computing": ["Edge", "Fog Computing", "Edge AI", "NVIDIA Jetson", "Raspberry Pi", "Gateway"],
    "Industrial Protocols": ["MQTT", "OPC-UA", "Modbus", "Profibus", "Profinet", "EtherCAT", "CAN", "EtherNet/IP"],
    "Data/Analytics": ["Python", "Kafka", "InfluxDB", "Grafana", "Time Series", "Machine Learning", "TensorFlow"],
    "Security": ["OT Security", "ICS Security", "Zero Trust", "Network Segmentation", "Firewall", "NIST"],
    "Linux/OS": ["Linux", "Yocto", "Buildroot", "Ubuntu", "Real-time Linux", "Windows IoT"]
}

def get_user_search_terms():
    """Interactive prompt for user to input search topics."""
    print("\n" + "="*60)
    print("🔧 IT/OT CONVERGENCE JOB SEARCH TOOL")
    print("="*60)
    print("\nDefault search terms:")
    for i, term in enumerate(DEFAULT_SEARCH_TERMS, 1):
        print(f"  {i}. {term}")
    
    print("\n📝 Options:")
    print("  [Enter] - Use default search terms")
    print("  [c]     - Enter custom search terms")
    print("  [a]     - Add to default terms")
    
    choice = input("\nYour choice: ").strip().lower()
    
    if choice == 'c':
        print("\nEnter your search terms (comma-separated):")
        print("Example: Docker Engineer, Kubernetes PLC, Edge Computing")
        custom = input("> ").strip()
        if custom:
            return [term.strip() for term in custom.split(',') if term.strip()]
        return DEFAULT_SEARCH_TERMS
    
    elif choice == 'a':
        print("\nEnter additional search terms (comma-separated):")
        additional = input("> ").strip()
        if additional:
            extra_terms = [term.strip() for term in additional.split(',') if term.strip()]
            return DEFAULT_SEARCH_TERMS + extra_terms
        return DEFAULT_SEARCH_TERMS
    
    else:
        return DEFAULT_SEARCH_TERMS

def get_user_countries():
    """Interactive prompt for country selection."""
    print("\n🌍 Available countries:")
    for i, country in enumerate(COUNTRIES, 1):
        print(f"  {i}. {country}")
    
    print("\n📝 Options:")
    print("  [Enter] - Search all countries")
    print("  [1,3,5] - Select specific countries by number")
    
    choice = input("\nYour choice: ").strip()
    
    if choice:
        try:
            indices = [int(x.strip()) - 1 for x in choice.split(',')]
            selected = [COUNTRIES[i] for i in indices if 0 <= i < len(COUNTRIES)]
            if selected:
                return selected
        except (ValueError, IndexError):
            pass
    
    return COUNTRIES

def clean_salary(salary_str):
    """Extracts yearly salary from strings like '€50k - €70k'."""
    if pd.isna(salary_str): return None
    
    matches = re.findall(r'(\d+[k]?)', str(salary_str).replace('.', '').replace(',', ''))
    
    clean_nums = []
    for m in matches:
        m = m.lower()
        if 'k' in m:
            clean_nums.append(float(m.replace('k', '')) * 1000)
        else:
            clean_nums.append(float(m))
    
    valid_nums = [n for n in clean_nums if 20000 < n < 200000]
    
    if valid_nums:
        return sum(valid_nums) / len(valid_nums)
    return None

def extract_skills(description):
    """Scans description for IT/OT convergence keywords."""
    found_skills = []
    if pd.isna(description): return found_skills
    
    desc_lower = description.lower()
    
    for category, skills in SKILL_KEYWORDS.items():
        for skill in skills:
            if re.search(r'\b' + re.escape(skill.lower()) + r'\b', desc_lower):
                found_skills.append(skill)
    return found_skills

def extract_skill_categories(description):
    """Returns which skill categories are present in the job."""
    found_categories = []
    if pd.isna(description): return found_categories
    
    desc_lower = description.lower()
    
    for category, skills in SKILL_KEYWORDS.items():
        for skill in skills:
            if re.search(r'\b' + re.escape(skill.lower()) + r'\b', desc_lower):
                if category not in found_categories:
                    found_categories.append(category)
                break
    return found_categories

def generate_mock_data(countries, search_terms):
    """Generates synthetic IT/OT convergence data."""
    print("⚠️  Generating MOCK DATA for visualization demonstration...")
    mock_jobs = []
    titles = [
        "IT/OT Integration Engineer", "Edge Computing Specialist", 
        "Industrial IoT Developer", "PLC Cloud Engineer",
        "Smart Factory Architect", "OT Security Analyst"
    ]
    cities = {
        "Germany": "Munich", "Netherlands": "Eindhoven", "France": "Lyon",
        "Sweden": "Gothenburg", "Finland": "Espoo", "Norway": "Oslo"
    }
    
    for _ in range(80):
        country = random.choice(countries)
        mock_jobs.append({
            "title": random.choice(titles),
            "company": f"IndustrialTech {random.randint(1, 100)}",
            "location": f"{cities.get(country, 'City')}, {country}",
            "country": country,
            "description": "Looking for IT/OT convergence expert with skills in C++, Python, Docker, Kubernetes, AWS, PLC programming, SCADA, OPC-UA, MQTT, Edge computing, and Linux. Experience with Siemens TIA Portal is a plus.",
            "job_url": "https://example.com",
            "site": "linkedin",
            "salary_clean": random.randint(55000, 95000)
        })
    return pd.DataFrame(mock_jobs)

def run_crawler(countries, search_terms):
    """Crawls job sites for IT/OT convergence positions."""
    all_jobs = []
    
    # Glassdoor doesn't support Nordic countries
    NORDIC_COUNTRIES = ['Sweden', 'Finland', 'Norway', 'Denmark', 'Iceland']
    
    print(f"\n🚀 Starting Crawl for: {search_terms}")
    print(f"🌍 Target Countries: {countries}")
    print("-" * 60)

    for country in countries:
        # Choose job sites based on country availability
        if country in NORDIC_COUNTRIES:
            # Nordic: Use Indeed + LinkedIn (no Glassdoor)
            sites = ["indeed", "linkedin"]
            print(f"\n   📍 {country} (using Indeed + LinkedIn)")
        else:
            # Other EU: Use Indeed + Glassdoor
            sites = ["indeed", "glassdoor"]
            print(f"\n   📍 {country} (using Indeed + Glassdoor)")
        
        for term in search_terms:
            print(f"      > Scraping '{term}'...")
            try:
                jobs = scrape_jobs(
                    site_name=sites,
                    search_term=term,
                    location=country,
                    results_wanted=RESULTS_WANTED,
                    country_indeed=country
                )
                
                if not jobs.empty:
                    jobs['country'] = country
                    jobs['search_term'] = term
                    all_jobs.append(jobs)
                    print(f"        ✅ Found {len(jobs)} jobs.")
                else:
                    print(f"        ❌ No jobs found.")
                
                time.sleep(3)
                
            except Exception as e:
                # If LinkedIn also fails, try Indeed only
                try:
                    print(f"        ⚠️ Retrying with Indeed only...")
                    jobs = scrape_jobs(
                        site_name=["indeed"],
                        search_term=term,
                        location=country,
                        results_wanted=RESULTS_WANTED,
                        country_indeed=country
                    )
                    if not jobs.empty:
                        jobs['country'] = country
                        jobs['search_term'] = term
                        all_jobs.append(jobs)
                        print(f"        ✅ Found {len(jobs)} jobs (Indeed only).")
                    else:
                        print(f"        ❌ No jobs found.")
                except Exception as e2:
                    print(f"        ❌ Failed: {e2}")
    
    if all_jobs:
        return pd.concat(all_jobs, ignore_index=True)
    return pd.DataFrame()

# --- MAIN EXECUTION ---
if __name__ == "__main__":
    
    # 1. GET USER INPUT
    search_terms = get_user_search_terms()
    countries = get_user_countries()
    
    print(f"\n✅ Search configuration:")
    print(f"   Terms: {search_terms}")
    print(f"   Countries: {countries}")
    
    # 2. ACQUIRE DATA
    df = run_crawler(countries, search_terms)
    
    if df.empty:
        if USE_MOCK_DATA_ON_FAIL:
            df = generate_mock_data(countries, search_terms)
        else:
            print("No data found. Exiting.")
            exit()
            
    print(f"\n📊 Processing {len(df)} total job listings...")

    # 3. PROCESS DATA
    if 'salary' in df.columns:
        df['salary_clean'] = df['salary'].apply(clean_salary)
    elif 'min_amount' in df.columns:
        df['salary_clean'] = df[['min_amount', 'max_amount']].mean(axis=1)
    else:
        df['salary_clean'] = None

    df['extracted_skills'] = df['description'].apply(extract_skills)
    df['skill_categories'] = df['description'].apply(extract_skill_categories)

    skills_expanded = df.explode('extracted_skills')
    categories_expanded = df.explode('skill_categories')

    # 4. VISUALIZE DATA
    
    # Chart 1: Top In-Demand Skills
    skill_counts = skills_expanded['extracted_skills'].value_counts().reset_index()
    skill_counts.columns = ['Skill', 'Count']
    
    fig_skills = px.bar(
        skill_counts.head(20), 
        x='Count', 
        y='Skill', 
        orientation='h',
        title='Top 20 IT/OT Convergence Skills in Demand',
        color='Count',
        color_continuous_scale='Viridis'
    )
    fig_skills.update_layout(yaxis={'categoryorder': 'total ascending'})
    
    # Chart 2: Skill Categories Distribution
    cat_counts = categories_expanded['skill_categories'].value_counts().reset_index()
    cat_counts.columns = ['Category', 'Count']
    
    fig_categories = px.pie(
        cat_counts,
        values='Count',
        names='Category',
        title='IT/OT Skill Categories Distribution',
        color_discrete_sequence=px.colors.qualitative.Set2
    )
    
    # Chart 3: Salary Distribution by Country
    # Nordic countries often don't publish salaries - add market estimates
    NORDIC_SALARY_ESTIMATES = {
        'Sweden': {'min': 55000, 'max': 85000, 'avg': 68000},      # SEK converted to EUR
        'Finland': {'min': 50000, 'max': 80000, 'avg': 63000},     # EUR
        'Norway': {'min': 65000, 'max': 95000, 'avg': 78000},      # NOK converted to EUR
    }
    
    salary_df = df.dropna(subset=['salary_clean'])
    
    # Check which countries have no salary data
    countries_with_data = salary_df['country'].unique() if not salary_df.empty else []
    countries_missing = [c for c in df['country'].unique() if c not in countries_with_data]
    
    # Add estimated data for Nordic countries
    estimated_rows = []
    for country in countries_missing:
        if country in NORDIC_SALARY_ESTIMATES:
            est = NORDIC_SALARY_ESTIMATES[country]
            # Create synthetic data points for visualization
            for salary in [est['min'], est['avg'], est['avg'], est['avg'], est['max']]:
                estimated_rows.append({
                    'country': f"{country} (est.)",
                    'salary_clean': salary,
                    'is_estimated': True
                })
    
    if estimated_rows:
        estimated_df = pd.DataFrame(estimated_rows)
        if not salary_df.empty:
            salary_df = salary_df.copy()
            salary_df['is_estimated'] = False
            combined_df = pd.concat([
                salary_df[['country', 'salary_clean', 'is_estimated']], 
                estimated_df
            ], ignore_index=True)
        else:
            combined_df = estimated_df
    else:
        combined_df = salary_df.copy() if not salary_df.empty else pd.DataFrame()
        if not combined_df.empty:
            combined_df['is_estimated'] = False
    
    if not combined_df.empty:
        fig_salary = px.box(
            combined_df, 
            x='country', 
            y='salary_clean', 
            title='Salary Distribution by Country (EUR) - Nordic data estimated from market research',
            points="all",
            color='country'
        )
        fig_salary.update_layout(
            xaxis_title="Country",
            yaxis_title="Yearly Salary (EUR)",
            annotations=[dict(
                text="Note: Nordic countries (Sweden, Finland, Norway) show estimated ranges - actual salaries are rarely published",
                xref="paper", yref="paper",
                x=0.5, y=-0.15,
                showarrow=False,
                font=dict(size=10, color="gray")
            )]
        )
    else:
        fig_salary = go.Figure().add_annotation(text="Not enough salary data", showarrow=False)

    # Chart 4: Job Volume Heatmap
    job_counts = df['country'].value_counts().reset_index()
    job_counts.columns = ['Country', 'Jobs Found']
    fig_map = px.choropleth(
        job_counts,
        locations='Country', 
        locationmode='country names',
        color='Jobs Found',
        scope='europe',
        title='IT/OT Convergence Jobs Heatmap (Europe)',
        color_continuous_scale='Plasma'
    )

    # Chart 5: Jobs by Search Term
    if 'search_term' in df.columns:
        term_counts = df['search_term'].value_counts().reset_index()
        term_counts.columns = ['Search Term', 'Count']
        fig_terms = px.bar(
            term_counts,
            x='Count',
            y='Search Term',
            orientation='h',
            title='Jobs Found by Search Term',
            color='Count',
            color_continuous_scale='Blues'
        )
        fig_terms.update_layout(yaxis={'categoryorder': 'total ascending'})
    else:
        fig_terms = go.Figure().add_annotation(text="No search term data", showarrow=False)

    # Save charts as HTML files
    fig_skills.write_html("skills_chart.html")
    fig_categories.write_html("categories_chart.html")
    fig_salary.write_html("salary_chart.html")
    fig_map.write_html("job_map.html")
    fig_terms.write_html("search_terms_chart.html")

    # Save data to CSV
    df.to_csv("itot_convergence_jobs.csv", index=False)
    
    print("\n" + "="*60)
    print("✅ Done! Files saved:")
    print("   - itot_convergence_jobs.csv")
    print("   - skills_chart.html")
    print("   - categories_chart.html")
    print("   - salary_chart.html")
    print("   - job_map.html")
    print("   - search_terms_chart.html")
    print("="*60)
    print("\n🌐 Starting web server on http://192.168.50.3:8000")
    print("   Open the HTML files in your browser.")
    print("   Press Ctrl+C to stop the server.")
    
    import http.server
    import socketserver
    
    Handler = http.server.SimpleHTTPRequestHandler
    with socketserver.TCPServer(("0.0.0.0", 8000), Handler) as httpd:
        httpd.serve_forever()
