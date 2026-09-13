const fs = require('fs');
const file = 'L:/Coding/Ellohim/Ellohim-Server/ellohim.sql';
let content = fs.readFileSync(file, 'utf8');

const tableSql = `
CREATE TABLE IF NOT EXISTS client_releases (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    version VARCHAR(50) NOT NULL,
    release_notes TEXT,
    file_name VARCHAR(255) NOT NULL,
    file_size BIGINT NOT NULL,
    checksum VARCHAR(64) NOT NULL,
    storage_path TEXT NOT NULL,
    is_mandatory BOOLEAN DEFAULT FALSE,
    min_supported_version VARCHAR(50) DEFAULT NULL,
    status VARCHAR(50) DEFAULT 'active',
    download_count BIGINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS client_modules (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    version VARCHAR(50) NOT NULL,
    target_path VARCHAR(255) NOT NULL,
    file_name VARCHAR(255) NOT NULL,
    file_size BIGINT NOT NULL,
    checksum VARCHAR(64) NOT NULL,
    storage_path TEXT NOT NULL,
    is_required BOOLEAN DEFAULT TRUE,
    status VARCHAR(50) DEFAULT 'active',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
`;

if (!content.includes('CREATE TABLE IF NOT EXISTS client_releases') && !content.includes('CREATE TABLE client_releases')) {
  content = content.replace('CREATE TABLE discounts (', tableSql + '\nCREATE TABLE discounts (');
  fs.writeFileSync(file, content, 'utf8');
  console.log('Inserted table definitions into ellohim.sql successfully');
} else {
  console.log('Table definitions already present in ellohim.sql');
}
