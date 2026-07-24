async function fetchLatestRelease() {
  const container = document.getElementById('latest-release-info');
  
  try {
    // Fetches the latest release payload directly from your public GitHub repository
    const response = await fetch('https://api.github.com/repos/pycity-project/pycity-project.github.io/releases/latest');
    
    if (!response.ok) throw new Error('Failed to fetch release metadata');
    
    const data = await response.json();
    
    // Formats line breaks from GitHub markdown description into clean web spaces
    const cleanBody = data.body ? data.body.replace(/\r\n|\n/g, '<br>') : 'No release notes provided.';
    
    // Dynamic output block matching your existing CSS variables
    container.innerHTML = `
      <div style="margin: 20px 0; padding: 15px; border: 1px solid var(--border-color); border-radius: 6px; background-color: var(--btn-bg);">
        <strong style="color: var(--text-color); font-size: 16px;">Latest Version: ${data.name || data.tag_name}</strong>
        <p style="color: var(--text-color); margin-top: 8px; font-size: 14px; line-height: 1.5;">${cleanBody}</p>
      </div>
    `;
  } catch (error) {
    console.error(error);
    container.innerHTML = `<p style="color: red;">Could not load version notes automatically.</p>`;
  }
}

// Executes fetch when document finishes structuring
document.addEventListener('DOMContentLoaded', fetchLatestRelease);
